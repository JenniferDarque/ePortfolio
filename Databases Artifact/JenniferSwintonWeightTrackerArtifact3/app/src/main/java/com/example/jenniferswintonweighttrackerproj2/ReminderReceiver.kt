package com.example.jenniferswintonweighttrackerproj2

import android.Manifest
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.telephony.SmsManager
import androidx.core.content.ContextCompat
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import java.text.SimpleDateFormat
import java.util.*

class ReminderReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        val smsPrefs = context.getSharedPreferences("SmsPrefs", Context.MODE_PRIVATE)
        val isReminderEnabled = smsPrefs.getBoolean("daily_reminder", false)

        if (isReminderEnabled) {
            val userPrefs = context.getSharedPreferences("UserPrefs", Context.MODE_PRIVATE)
            val currentUsername = userPrefs.getString("current_username", "") ?: ""

            if (currentUsername.isNotEmpty()) {
                val db = AppDatabase.getDatabase(context)
                val pendingResult = goAsync()
                
                CoroutineScope(Dispatchers.IO).launch {
                    try {
                        val allEntries = db.weightEntryDao().getAllEntries(currentUsername)

                        // Get today's date in the app's format
                        val sdf = SimpleDateFormat("MM/dd/yyyy", Locale.US)
                        val todayStr = sdf.format(Date())

                        // Check if there is an entry for today
                        val hasEntryForToday = allEntries.any { it.date == todayStr }

                        if (!hasEntryForToday) {
                            sendSms(context, "Reminder: You haven't logged your weight today.")
                        }
                    } finally {
                        pendingResult.finish()
                    }
                }
            }
        }
    }

    private fun sendSms(context: Context, message: String) {
        if (ContextCompat.checkSelfPermission(context, Manifest.permission.SEND_SMS) == PackageManager.PERMISSION_GRANTED) {
            try {
                val smsManager = context.getSystemService(SmsManager::class.java)
                smsManager?.sendTextMessage("3179952109", null, message, null, null)
            } catch (_: Exception) {
                // Handle failure (e.g. logging)
            }
        }
    }
}
