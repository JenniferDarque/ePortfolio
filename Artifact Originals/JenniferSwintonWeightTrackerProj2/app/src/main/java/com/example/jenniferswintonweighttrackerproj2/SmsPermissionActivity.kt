package com.example.jenniferswintonweighttrackerproj2

import android.app.AlarmManager
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Bundle
import android.view.View
import android.widget.Button
import android.widget.LinearLayout
import android.widget.Switch
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.core.content.ContextCompat
import java.util.*

class SmsPermissionActivity : ComponentActivity() {

    private lateinit var layoutToggles: LinearLayout
    private lateinit var switchGoalReached: Switch
    private lateinit var switchDailyReminder: Switch

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_sms_permission)

        layoutToggles = findViewById(R.id.layoutToggles)
        switchGoalReached = findViewById(R.id.switchGoalReached)
        switchDailyReminder = findViewById(R.id.switchDailyReminder)

        val btnCheckPermission = findViewById<Button>(R.id.btnCheckPermission)
        val btnReturnHome = findViewById<Button>(R.id.btnReturnHome)
        val btnLogout = findViewById<Button>(R.id.btnLogout)

        // Load saved states
        val sharedPref = getSharedPreferences("SmsPrefs", Context.MODE_PRIVATE)
        switchGoalReached.isChecked = sharedPref.getBoolean("goal_reached", false)
        switchDailyReminder.isChecked = sharedPref.getBoolean("daily_reminder", false)

        // Save states on change
        switchGoalReached.setOnCheckedChangeListener { _, isChecked ->
            sharedPref.edit().putBoolean("goal_reached", isChecked).apply()
        }
        switchDailyReminder.setOnCheckedChangeListener { _, isChecked ->
            sharedPref.edit().putBoolean("daily_reminder", isChecked).apply()
            updateReminderAlarm(isChecked)
        }

        btnCheckPermission.setOnClickListener {
            checkSmsPermission()
        }

        btnReturnHome.setOnClickListener {
            finish()
        }

        btnLogout.setOnClickListener {
            val intent = Intent(this, MainActivity::class.java)
            intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK
            startActivity(intent)
            finish()
        }

        // Hide toggles initially
        layoutToggles.visibility = View.GONE
    }

    private fun checkSmsPermission() {
        if (ContextCompat.checkSelfPermission(this, android.Manifest.permission.SEND_SMS) == PackageManager.PERMISSION_GRANTED) {
            Toast.makeText(this, "Notifications turned on", Toast.LENGTH_SHORT).show()
            layoutToggles.visibility = View.VISIBLE
        } else {
            Toast.makeText(this, "Notifications turned off", Toast.LENGTH_SHORT).show()
            layoutToggles.visibility = View.GONE
        }
    }

    private fun updateReminderAlarm(isEnabled: Boolean) {
        val alarmManager = getSystemService(Context.ALARM_SERVICE) as AlarmManager
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
