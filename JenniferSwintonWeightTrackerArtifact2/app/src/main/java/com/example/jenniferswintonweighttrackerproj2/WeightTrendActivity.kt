package com.example.jenniferswintonweighttrackerproj2

import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.text.InputType
import android.view.View
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.launch
import java.text.SimpleDateFormat
import java.util.*
import kotlin.math.abs
import kotlin.math.roundToInt

class WeightTrendActivity : AppCompatActivity() {

    private lateinit var graphView: GraphView
    private lateinit var tvGoalWeight: TextView
    private lateinit var tvPrediction: TextView
    private lateinit var tvAccuracyWarning: TextView
    private var allEntries: List<WeightEntry> = emptyList()
    private lateinit var db: AppDatabase
    private val dateFormat = SimpleDateFormat("MM/dd/yyyy", Locale.US)
    private var predictionDays = 30

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_weight_trend)

        db = AppDatabase.getDatabase(this)
        graphView = findViewById(R.id.graphView)
        tvGoalWeight = findViewById(R.id.tvGoalWeight)
        tvPrediction = findViewById(R.id.tvPrediction)
        tvAccuracyWarning = findViewById(R.id.tvAccuracyWarning)
        
        val btnSetGoal = findViewById<Button>(R.id.btnSetGoal)
        val btnEnterWeight = findViewById<Button>(R.id.btnEnterWeight)
        val btnSmsSettings = findViewById<Button>(R.id.btnSmsSettings)
        val btnLogout = findViewById<Button>(R.id.btnLogout)

        btnSetGoal.setOnClickListener {
            val intent = Intent(this, SetGoalActivity::class.java)
            startActivity(intent)
        }

        btnEnterWeight.setOnClickListener {
            val intent = Intent(this, DataGridActivity::class.java)
            startActivity(intent)
        }

        btnSmsSettings.setOnClickListener {
            val intent = Intent(this, SmsPermissionActivity::class.java)
            startActivity(intent)
        }

        btnLogout.setOnClickListener {
            val intent = Intent(this, MainActivity::class.java)
            intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK
            startActivity(intent)
            finish()
        }

        tvPrediction.setOnClickListener {
            showChangeDaysDialog()
        }
    }

    override fun onResume() {
        super.onResume()
        // Refresh goal weight display
        val sharedPref = getSharedPreferences("UserPrefs", Context.MODE_PRIVATE)
        val goal = sharedPref.getString("goal_weight", "---")
        tvGoalWeight.text = getString(R.string.goal_weight_display, goal)
        
        // Refresh data from database
        refreshData()
    }

    private fun refreshData() {
        val sharedPref = getSharedPreferences("UserPrefs", Context.MODE_PRIVATE)
        val currentUsername = sharedPref.getString("current_username", "") ?: ""
        
        lifecycleScope.launch {
            allEntries = db.weightEntryDao().getAllEntries(currentUsername)
            // Sort entries by date to ensure the graph draws correctly (Oldest first for graph path)
            val sortedEntries = allEntries.sortedBy { 
                try { dateFormat.parse(it.date) } catch (_: Exception) { Date(0) }
            }
            
            val historicalWeights = sortedEntries.mapNotNull { it.weight.toDoubleOrNull() }
            val predictiveWeights = updatePredictionAndGetPoints(sortedEntries)
            
            graphView.setData(historicalWeights, predictiveWeights, false)
        }
    }

    private fun updatePredictionAndGetPoints(entries: List<WeightEntry>): List<Double> {
        val sharedPref = getSharedPreferences("UserPrefs", Context.MODE_PRIVATE)
        val goalStr = sharedPref.getString("goal_weight", null)
        val goalWeight = goalStr?.toDoubleOrNull()

        val now = Calendar.getInstance()
        val thirtyDaysAgo = Calendar.getInstance().apply { 
            add(Calendar.DAY_OF_YEAR, -30)
            set(Calendar.HOUR_OF_DAY, 0)
            set(Calendar.MINUTE, 0)
            set(Calendar.SECOND, 0)
            set(Calendar.MILLISECOND, 0)
        }

        // Filter entries for the last 30 calendar days to build the trend model
        val recentEntries = entries.filter {
            try {
                val entryDate = dateFormat.parse(it.date)
                entryDate != null && entryDate.time >= thirtyDaysAgo.timeInMillis
            } catch (_: Exception) {
                false
            }
        }

        if (recentEntries.size < 14) {
            tvPrediction.text = getString(R.string.trend_need_more_data)
            tvAccuracyWarning.visibility = View.GONE
            return emptyList()
        }

        val weights = recentEntries.mapNotNull { it.weight.toDoubleOrNull() }
        val dates = recentEntries.mapNotNull { 
            try { dateFormat.parse(it.date) } catch (_: Exception) { null } 
        }

        if (weights.size < 14 || dates.size != weights.size) {
            tvPrediction.text = getString(R.string.trend_need_more_data)
            tvAccuracyWarning.visibility = View.GONE
            return emptyList()
        }

        // 1. Smoothing: 3-point moving average
        val smoothedWeights = DoubleArray(weights.size)
        for (i in weights.indices) {
            val start = maxOf(0, i - 1)
            val end = minOf(weights.size - 1, i + 1)
            var sum = 0.0
            var count = 0
            for (j in start..end) {
                sum += weights[j]
                count++
            }
            smoothedWeights[i] = sum / count
        }

        // 2. Linear Regression (y = mx + b)
        val startTime = dates.first().time
        val xValues = dates.map { (it.time - startTime).toDouble() / (24 * 60 * 60 * 1000) }

        val n = xValues.size
        var sumX = 0.0
        var sumY = 0.0
        var sumXY = 0.0
        var sumXX = 0.0

        for (i in 0 until n) {
            sumX += xValues[i]
            sumY += smoothedWeights[i]
            sumXY += xValues[i] * smoothedWeights[i]
            sumXX += xValues[i] * xValues[i]
        }

        val denominator = n * sumXX - sumX * sumX
        if (abs(denominator) < 1e-10) {
            tvPrediction.text = getString(R.string.trend_stable)
            tvAccuracyWarning.visibility = View.GONE
            return emptyList()
        }

        val slope = (n * sumXY - sumX * sumY) / denominator
        val intercept = (sumY - slope * sumX) / n

        // 3. Predict weight for user-defined days from now
        val daysFromStartToNow = (now.timeInMillis - startTime).toDouble() / (24 * 60 * 60 * 1000)
        
        // Generate predictive points for the graph
        val predictivePoints = mutableListOf<Double>()
        val steps = 3 // Points to define the dotted line
        for (i in 1..steps) {
            val futureDay = daysFromStartToNow + (predictionDays.toDouble() * i / steps)
            var pred = slope * futureDay + intercept
            
            // Safeguards
            val lastActualWeight = weights.last()
            val changeFactor = 0.2 + (predictionDays.toDouble() / 365.0) * 0.3
            val maxReasonableChange = maxOf(50.0, lastActualWeight * changeFactor)
            pred = pred.coerceIn(lastActualWeight - maxReasonableChange, lastActualWeight + maxReasonableChange)
            pred = pred.coerceIn(50.0, 1000.0)
            
            predictivePoints.add(pred)
        }

        val finalPredictedWeight = predictivePoints.last()
        val currentDateStr = SimpleDateFormat("MM/dd/yyyy", Locale.US).format(now.time)
        val resultText = StringBuilder()
        resultText.append(getString(R.string.weight_prediction_result, predictionDays, currentDateStr, finalPredictedWeight))

        // Check if goal will be reached
        if (goalWeight != null) {
            val currentTrendWeight = slope * daysFromStartToNow + intercept
            val isReachingGoal = (slope < 0 && finalPredictedWeight <= goalWeight && currentTrendWeight > goalWeight) || 
                                 (slope > 0 && finalPredictedWeight >= goalWeight && currentTrendWeight < goalWeight)
            
            if (isReachingGoal) {
                val daysToGoal = (goalWeight - intercept) / slope - daysFromStartToNow
                if (daysToGoal > 0) {
                    resultText.append("\n")
                    resultText.append(getString(R.string.goal_reach_prediction, daysToGoal.roundToInt()))
                }
            }
        }

        tvPrediction.text = resultText.toString()

        // Show warning if more than 3 months (90 days)
        tvAccuracyWarning.visibility = if (predictionDays > 90) View.VISIBLE else View.GONE
        
        return predictivePoints
    }

    private fun showChangeDaysDialog() {
        val builder = AlertDialog.Builder(this)
        builder.setTitle(R.string.change_prediction_days)
        builder.setMessage(R.string.enter_days_up_to_12_months)

        val input = EditText(this)
        input.inputType = InputType.TYPE_CLASS_NUMBER
        input.setText(predictionDays.toString())
        builder.setView(input)

        builder.setPositiveButton(R.string.ok) { _, _ ->
            val value = input.text.toString().toIntOrNull()
            if (value != null && value in 1..365) {
                predictionDays = value
                refreshData()
            } else {
                Toast.makeText(this, R.string.invalid_input, Toast.LENGTH_SHORT).show()
            }
        }
        builder.setNegativeButton(R.string.cancel, null)
        builder.show()
    }
}
