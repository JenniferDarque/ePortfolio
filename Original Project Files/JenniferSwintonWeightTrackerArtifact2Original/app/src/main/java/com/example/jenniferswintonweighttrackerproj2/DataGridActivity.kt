package com.example.jenniferswintonweighttrackerproj2

import android.Manifest
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.telephony.SmsManager
import android.view.View
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import kotlin.math.abs

class DataGridActivity : ComponentActivity() {

    private lateinit var adapter: WeightAdapter
    private val weightEntries = mutableListOf<WeightEntry>()
    private lateinit var dbHelper: DatabaseHelper
    private val alertHandler = Handler(Looper.getMainLooper())
    private var alertRunnable: Runnable? = null
    private var currentUsername: String = ""

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_data_grid)

        // Retrieve the current user's username
        val userPrefs = getSharedPreferences("UserPrefs", Context.MODE_PRIVATE)
        currentUsername = userPrefs.getString("current_username", "") ?: ""

        dbHelper = DatabaseHelper(this)
        
        // Load entries specific to the current user
        weightEntries.addAll(dbHelper.getAllEntries(currentUsername))

        val tvWeightAlert = findViewById<TextView>(R.id.tvWeightAlert)

        // Setup RecyclerView
        val recyclerView = findViewById<RecyclerView>(R.id.rvWeightEntries)
        adapter = WeightAdapter(weightEntries, { entry ->
            // Delete callback
            dbHelper.deleteEntry(entry.id)
            weightEntries.remove(entry)
            adapter.notifyDataSetChanged()
            checkWeightChange(tvWeightAlert)
        }, { entry, newDate, newWeight ->
            // Update callback
            dbHelper.updateWeightEntry(entry.id, newDate, newWeight)
            val index = weightEntries.indexOf(entry)
            if (index != -1) {
                weightEntries[index] = WeightEntry(entry.id, newDate, newWeight, currentUsername)
                // Re-sort the list to keep most recent on top after an update
                weightEntries.sortByDescending { it.id }
                adapter.notifyDataSetChanged()
                checkWeightChange(tvWeightAlert)
                checkGoalReached(newWeight)
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

        // Update Goal Weight Navigation (Replaces View Trend)
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

        btnAdd.setOnClickListener {
            val date = etDate.text.toString()
            val weightStr = etWeight.text.toString()

            if (date.isNotEmpty() && weightStr.isNotEmpty()) {
                val id = dbHelper.addWeightEntry(date, weightStr, currentUsername)
                val newEntry = WeightEntry(id.toInt(), date, weightStr, currentUsername)
                // Add new entry to the top of the list
                weightEntries.add(0, newEntry)
                adapter.notifyDataSetChanged()
                
                checkWeightChange(tvWeightAlert)
                checkGoalReached(weightStr)
                
                etDate.text.clear()
                etWeight.text.clear()
            }
        }
    }

    private fun checkWeightChange(tvWeightAlert: TextView) {
        // Clear any existing timer to avoid overlapping hides
        alertRunnable?.let { alertHandler.removeCallbacks(it) }

        // Since the list is now sorted most recent first, latest is at index 0, previous is at index 1
        if (weightEntries.size >= 2) {
            val latestWeight = weightEntries[0].weight.toDoubleOrNull() ?: 0.0
            val previousWeight = weightEntries[1].weight.toDoubleOrNull() ?: 0.0
            
            val diff = abs(latestWeight - previousWeight)
            
            if (diff >= 10.0) {
                tvWeightAlert.visibility = View.VISIBLE
                
                // Requirement: Alert should only display for 5 seconds
                alertRunnable = Runnable {
                    tvWeightAlert.visibility = View.GONE
                }
                alertHandler.postDelayed(alertRunnable!!, 5000)
            } else {
                tvWeightAlert.visibility = View.GONE
            }
        } else {
            tvWeightAlert.visibility = View.GONE
        }
    }

    private fun checkGoalReached(enteredWeight: String) {
        // Check if SMS Goal Reached notification is enabled in app settings
        val smsPrefs = getSharedPreferences("SmsPrefs", Context.MODE_PRIVATE)
        val isGoalNotifyEnabled = smsPrefs.getBoolean("goal_reached", false)

        if (isGoalNotifyEnabled) {
            // Retrieve the goal weight
            val userPrefs = getSharedPreferences("UserPrefs", Context.MODE_PRIVATE)
            val goalWeightStr = userPrefs.getString("goal_weight", null)

            if (goalWeightStr != null) {
                val goalWeight = goalWeightStr.toDoubleOrNull()
                val currentWeight = enteredWeight.toDoubleOrNull()

                // Requirement: Congratulations message when goal weight = daily weight entry
                if (goalWeight != null && currentWeight != null && currentWeight <= goalWeight) {
                    sendSmsNotification("Congratulations! You have reached your target weight.")
                }
            }
        }
    }

    private fun sendSmsNotification(message: String) {
        // First, check if the app has system-level permission to send SMS
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.SEND_SMS) == PackageManager.PERMISSION_GRANTED) {
            try {
                val smsManager: SmsManager = this.getSystemService(SmsManager::class.java)
                // Use the provided phone number
                smsManager.sendTextMessage("3179952109", null, message, null, null)
                Toast.makeText(this, "Goal SMS Sent!", Toast.LENGTH_SHORT).show()
            } catch (e: Exception) {
                Toast.makeText(this, "Failed to send SMS", Toast.LENGTH_SHORT).show()
            }
        } else {
            // Fallback if system permission is missing but app toggle is on
            Toast.makeText(this, "SMS Permission not granted in system settings.", Toast.LENGTH_LONG).show()
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        // Clean up handler to prevent memory leaks
        alertRunnable?.let { alertHandler.removeCallbacks(it) }
    }
}
