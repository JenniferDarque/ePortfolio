package com.example.jenniferswintonweighttrackerproj2

import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.widget.Button
import android.widget.TextView
import androidx.activity.ComponentActivity
import java.text.SimpleDateFormat
import java.util.*

class WeightTrendActivity : ComponentActivity() {

    private lateinit var graphView: GraphView
    private lateinit var tvGoalWeight: TextView
    private var allEntries: List<WeightEntry> = emptyList()
    private lateinit var dbHelper: DatabaseHelper
    private val dateFormat = SimpleDateFormat("M/d/yyyy", Locale.US)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_weight_trend)

        dbHelper = DatabaseHelper(this)
        graphView = findViewById(R.id.graphView)
        tvGoalWeight = findViewById(R.id.tvGoalWeight)
        
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
        
        allEntries = dbHelper.getAllEntries(currentUsername)
        // Sort entries by date to ensure the graph draws correctly
        val sortedEntries = allEntries.sortedBy { 
            try { dateFormat.parse(it.date) } catch (e: Exception) { Date(0) }
        }
        updateGraph(sortedEntries)
    }

    private fun updateGraph(entries: List<WeightEntry>) {
        val weights = entries.mapNotNull { it.weight.toDoubleOrNull() }
        graphView.setData(weights, false)
    }
}
