package com.example.jenniferswintonweighttrackerproj2

import android.Manifest
import android.app.AlertDialog
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.telephony.SmsManager
import android.text.Editable
import android.text.TextWatcher
import android.view.View
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import kotlinx.coroutines.launch
import java.text.SimpleDateFormat
import java.util.Locale
import kotlin.math.abs

class DataGridActivity : AppCompatActivity() {

    private lateinit var adapter: WeightAdapter
    private val weightEntries = mutableListOf<WeightEntry>()
    private lateinit var db: AppDatabase
    private val alertHandler = Handler(Looper.getMainLooper())
    private var alertRunnable: Runnable? = null
    private var currentUsername: String = ""
    private val dateFormat = SimpleDateFormat("MM/dd/yyyy", Locale.US)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_data_grid)

        // Retrieve the current user's username
        val userPrefs = getSharedPreferences("UserPrefs", MODE_PRIVATE)
        currentUsername = userPrefs.getString("current_username", "") ?: ""

        db = AppDatabase.getDatabase(this)
        
        // Load entries specific to the current user
        loadAndSortEntries()

        val tvWeightAlert = findViewById<TextView>(R.id.tvWeightAlert)

        // Setup RecyclerView
        val recyclerView = findViewById<RecyclerView>(R.id.rvWeightEntries)
        adapter = WeightAdapter(weightEntries, { entry ->
            // Delete callback
            lifecycleScope.launch {
                db.weightEntryDao().deleteEntry(entry.id)
                weightEntries.remove(entry)
                adapter.notifyDataSetChanged()
                checkWeightChange(tvWeightAlert)
            }
        }, { entry, newDate, newWeight ->
            // Update callback
            val normalizedDate = normalizeDateInput(newDate) ?: newDate
            
            // Weight validation
            val weightValue = newWeight.toDoubleOrNull()
            if (weightValue == null || weightValue < 100.0 || weightValue > 1000.0) {
                Toast.makeText(this, "Weight entry failed. Please enter a value between 100 and 1000 lbs.", Toast.LENGTH_LONG).show()
                return@WeightAdapter
            }

            lifecycleScope.launch {
                val updatedEntry = WeightEntry(entry.id, normalizedDate, newWeight, currentUsername)
                db.weightEntryDao().updateWeightEntry(updatedEntry)
                val index = weightEntries.indexOf(entry)
                if (index != -1) {
                    weightEntries[index] = updatedEntry
                    sortWeightEntries()
                    adapter.notifyDataSetChanged()
                    checkWeightChange(tvWeightAlert)
                    checkGoalReached(newWeight)
                }
            }
        })
        recyclerView.adapter = adapter
        recyclerView.layoutManager = LinearLayoutManager(this)

        // Home Button Navigation (Goes to WeightTrendActivity)
        val btnHome = findViewById<Button>(R.id.btnHome)
        btnHome.setOnClickListener {
            val intent = Intent(this, WeightTrendActivity::class.java)
            intent.flags = Intent.FLAG_ACTIVITY_CLEAR_TOP
            startActivity(intent)
        }

        // Update Goal Weight Navigation
        val btnUpdateGoalWeight = findViewById<Button>(R.id.btnUpdateGoalWeight)
        btnUpdateGoalWeight.setOnClickListener {
            val intent = Intent(this, SetGoalActivity::class.java)
            startActivity(intent)
        }

        val btnLogout = findViewById<Button>(R.id.btnLogout)
        btnLogout.setOnClickListener {
            val intent = Intent(this, MainActivity::class.java)
            intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK
            startActivity(intent)
            finish()
        }

        // Add Entry logic
        val etDate = findViewById<EditText>(R.id.etNewDate)
        val etWeight = findViewById<EditText>(R.id.etNewWeight)
        val btnAdd = findViewById<Button>(R.id.btnAddEntry)

        setupDateAutoFormatter(etDate)

        btnAdd.setOnClickListener {
            val dateStr = etDate.text.toString().trim()
            val weightStr = etWeight.text.toString().trim()

            if (dateStr.isNotEmpty() && weightStr.isNotEmpty()) {
                val normalizedDate = normalizeDateInput(dateStr)
                if (normalizedDate == null) {
                    Toast.makeText(this, "Invalid date format. Please use MMDDYYYY or MM/DD/YYYY", Toast.LENGTH_SHORT).show()
                    return@setOnClickListener
                }

                // Ensure weight is clean (no slashes)
                val cleanWeight = sanitizeWeightInput(weightStr)
                
                val weightValue = cleanWeight.toDoubleOrNull()
                if (weightValue == null || weightValue < 100.0 || weightValue > 1000.0) {
                    Toast.makeText(this, "Weight entry failed. Please enter a value between 100 and 1000 lbs.", Toast.LENGTH_LONG).show()
                    return@setOnClickListener
                }

                val existingEntry = weightEntries.find { it.date == normalizedDate }
                if (existingEntry != null) {
                    showDuplicateDateDialog(existingEntry, cleanWeight, tvWeightAlert, etDate, etWeight)
                } else {
                    addNewEntry(normalizedDate, cleanWeight, tvWeightAlert)
                    etDate.text.clear()
                    etWeight.text.clear()
                }
            }
        }
    }

    private fun setupDateAutoFormatter(editText: EditText) {
        editText.addTextChangedListener(object : TextWatcher {
            private var isUpdating = false
            override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {}
            override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) {}
            override fun afterTextChanged(s: Editable?) {
                if (isUpdating) return
                isUpdating = true

                val input = s.toString().replace("/", "")
                val formatted = StringBuilder()
                for (i in input.indices) {
                    if (i == 2 || i == 4) {
                        formatted.append("/")
                    }
                    formatted.append(input[i])
                    if (formatted.length >= 10) break 
                }

                if (formatted.toString() != s.toString()) {
                    s?.replace(0, s.length, formatted.toString())
                    editText.setSelection(formatted.length)
                }
                isUpdating = false
            }
        })
    }

    private fun sanitizeWeightInput(input: String): String {
        return input.replace("/", "")
    }

    private fun normalizeDateInput(input: String): String? {
        val cleanInput = input.replace("/", "")
        return if (cleanInput.length == 8) {
            val mm = cleanInput.substring(0, 2)
            val dd = cleanInput.substring(2, 4)
            val yyyy = cleanInput.substring(4, 8)
            val formatted = "$mm/$dd/$yyyy"
            try {
                dateFormat.isLenient = false
                dateFormat.parse(formatted)
                formatted
            } catch (_: Exception) {
                null
            }
        } else {
            try {
                dateFormat.isLenient = false
                val date = dateFormat.parse(input)
                if (date != null) dateFormat.format(date) else null
            } catch (_: Exception) {
                null
            }
        }
    }

    private fun loadAndSortEntries() {
        lifecycleScope.launch {
            val entries = db.weightEntryDao().getAllEntries(currentUsername)
            weightEntries.clear()
            weightEntries.addAll(entries)
            sortWeightEntries()
            adapter.notifyDataSetChanged()
        }
    }

    private fun sortWeightEntries() {
        weightEntries.sortWith(compareByDescending<WeightEntry> { 
            try {
                dateFormat.parse(it.date)
            } catch (e: Exception) {
                null
            }
        }.thenByDescending { it.id })
    }

    private fun showDuplicateDateDialog(existingEntry: WeightEntry, newWeight: String, tvWeightAlert: TextView, etDate: EditText, etWeight: EditText) {
        AlertDialog.Builder(this)
            .setMessage(R.string.duplicate_date_warning)
            .setPositiveButton(R.string.yes) { _, _ ->
                lifecycleScope.launch {
                    val updatedEntry = WeightEntry(existingEntry.id, existingEntry.date, newWeight, currentUsername)
                    db.weightEntryDao().updateWeightEntry(updatedEntry)
                    val index = weightEntries.indexOf(existingEntry)
                    if (index != -1) {
                        weightEntries[index] = updatedEntry
                        sortWeightEntries()
                        adapter.notifyDataSetChanged()
                        checkWeightChange(tvWeightAlert)
                        checkGoalReached(newWeight)
                        etDate.text.clear()
                        etWeight.text.clear()
                    }
                }
            }
            .setNegativeButton(R.string.no, null)
            .show()
    }

    private fun addNewEntry(date: String, weightStr: String, tvWeightAlert: TextView) {
        lifecycleScope.launch {
            val entry = WeightEntry(date = date, weight = weightStr, username = currentUsername)
            val id = db.weightEntryDao().addWeightEntry(entry)
            val newEntry = entry.copy(id = id.toInt())
            weightEntries.add(newEntry)
            sortWeightEntries()
            adapter.notifyDataSetChanged()
            
            checkWeightChange(tvWeightAlert)
            checkGoalReached(weightStr)
        }
    }

    private fun checkWeightChange(tvWeightAlert: TextView) {
        alertRunnable?.let { alertHandler.removeCallbacks(it) }

        if (weightEntries.size >= 2) {
            val latestWeight = weightEntries[0].weight.toDoubleOrNull() ?: 0.0
            val previousWeight = weightEntries[1].weight.toDoubleOrNull() ?: 0.0
            
            val diff = abs(latestWeight - previousWeight)
            
            if (diff >= 10.0) {
                tvWeightAlert.visibility = View.VISIBLE
                alertRunnable = Runnable { tvWeightAlert.visibility = View.GONE }
                alertHandler.postDelayed(alertRunnable!!, 5000)
            } else {
                tvWeightAlert.visibility = View.GONE
            }
        } else {
            tvWeightAlert.visibility = View.GONE
        }
    }

    private fun checkGoalReached(enteredWeight: String) {
        val smsPrefs = getSharedPreferences("SmsPrefs", MODE_PRIVATE)
        val isGoalNotifyEnabled = smsPrefs.getBoolean("goal_reached", false)

        if (isGoalNotifyEnabled) {
            val userPrefs = getSharedPreferences("UserPrefs", MODE_PRIVATE)
            val goalWeightStr = userPrefs.getString("goal_weight", null)

            if (goalWeightStr != null) {
                val goalWeight = goalWeightStr.toDoubleOrNull()
                val currentWeight = enteredWeight.toDoubleOrNull() ?: 0.0

                if (goalWeight != null && currentWeight <= goalWeight) {
                    sendSmsNotification("Congratulations! You have reached your target weight.")
                }
            }
        }
    }

    private fun sendSmsNotification(message: String) {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.SEND_SMS) == PackageManager.PERMISSION_GRANTED) {
            try {
                val smsManager: SmsManager = this.getSystemService(SmsManager::class.java)
                smsManager.sendTextMessage("3179952109", null, message, null, null)
                Toast.makeText(this, "Goal SMS Sent!", Toast.LENGTH_SHORT).show()
            } catch (_: Exception) {
                Toast.makeText(this, "Failed to send SMS", Toast.LENGTH_SHORT).show()
            }
        } else {
            Toast.makeText(this, "SMS Permission not granted.", Toast.LENGTH_LONG).show()
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        alertRunnable?.let { alertHandler.removeCallbacks(it) }
    }
}
