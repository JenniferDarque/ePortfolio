package com.example.jenniferswintonweighttrackerproj2

import android.Manifest
import android.app.AlarmManager
import android.app.PendingIntent
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Bundle
import android.view.View
import android.widget.Button
import android.widget.LinearLayout
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.SwitchCompat
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.core.content.edit
import java.util.*

class SmsPermissionActivity : AppCompatActivity() {

    private lateinit var layoutToggles: LinearLayout
    private lateinit var switchGoalReached: SwitchCompat
    private lateinit var switchDailyReminder: SwitchCompat

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_sms_permission)

        layoutToggles = findViewById(R.id.layoutToggles)
        switchGoalReached = findViewById(R.id.switchGoalReached)
        switchDailyReminder = findViewById(R.id.switchDailyReminder)

        val btnCheckPermission = findViewById<Button>(R.id.btnCheckPermission)
        val btnReturnHome = findViewById<Button>(R.id.btnReturnHome)
        val btnLogoutBottom = findViewById<Button>(R.id.btnLogoutBottom)

        // Load saved states
        val sharedPref = getSharedPreferences("SmsPrefs", MODE_PRIVATE)
        switchGoalReached.isChecked = sharedPref.getBoolean("goal_reached", false)
        switchDailyReminder.isChecked = sharedPref.getBoolean("daily_reminder", false)

        // Save states on change using KTX
        switchGoalReached.setOnCheckedChangeListener { _, isChecked ->
            sharedPref.edit { putBoolean("goal_reached", isChecked) }
        }
        switchDailyReminder.setOnCheckedChangeListener { _, isChecked ->
            sharedPref.edit { putBoolean("daily_reminder", isChecked) }
            updateReminderAlarm(isChecked)
        }

        btnCheckPermission.setOnClickListener {
            checkSmsPermission()
        }

        btnReturnHome.setOnClickListener {
            finish()
        }

        btnLogoutBottom.setOnClickListener {
            val intent = Intent(this, MainActivity::class.java)
            intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK
            startActivity(intent)
            finish()
        }

        // Check permission initially to show/hide toggles
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.SEND_SMS) == PackageManager.PERMISSION_GRANTED) {
            layoutToggles.visibility = View.VISIBLE
        } else {
            layoutToggles.visibility = View.GONE
        }
    }

    private fun checkSmsPermission() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.SEND_SMS) == PackageManager.PERMISSION_GRANTED) {
            Toast.makeText(this, "Notifications turned on", Toast.LENGTH_SHORT).show()
            layoutToggles.visibility = View.VISIBLE
        } else {
            // Request permission if not granted
            ActivityCompat.requestPermissions(
                this,
                arrayOf(Manifest.permission.SEND_SMS),
                1
            )
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == 1) {
            if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                Toast.makeText(this, "Permission granted", Toast.LENGTH_SHORT).show()
                layoutToggles.visibility = View.VISIBLE
            } else {
                Toast.makeText(this, "Permission denied. Cannot send notifications.", Toast.LENGTH_LONG).show()
                layoutToggles.visibility = View.GONE
            }
        }
    }

    private fun updateReminderAlarm(isEnabled: Boolean) {
        val alarmManager = getSystemService(ALARM_SERVICE) as AlarmManager
        val intent = Intent(this, ReminderReceiver::class.java)
        val pendingIntent = PendingIntent.getBroadcast(
            this,
            0,
            intent,
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )

        if (isEnabled) {
            val calendar = Calendar.getInstance().apply {
                timeInMillis = System.currentTimeMillis()
                set(Calendar.HOUR_OF_DAY, 9)
                set(Calendar.MINUTE, 0)
                set(Calendar.SECOND, 0)
                set(Calendar.MILLISECOND, 0)
                
                // If it's already past 9 AM, schedule the first alarm for tomorrow
                if (before(Calendar.getInstance())) {
                    add(Calendar.DAY_OF_YEAR, 1)
                }
            }

            alarmManager.setInexactRepeating(
                AlarmManager.RTC_WAKEUP,
                calendar.timeInMillis,
                AlarmManager.INTERVAL_DAY,
                pendingIntent
            )
        } else {
            alarmManager.cancel(pendingIntent)
        }
    }
}
