package com.example.jenniferswintonweighttrackerproj2

import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.widget.Button
import android.widget.EditText
import android.widget.Toast
import androidx.activity.ComponentActivity

class SetGoalActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_set_goal)

        val etGoalWeight = findViewById<EditText>(R.id.etGoalWeight)
        val btnUpdate = findViewById<Button>(R.id.btnUpdateGoal)
        val btnCancel = findViewById<Button>(R.id.btnCancelGoal)
        val btnLogout = findViewById<Button>(R.id.btnLogout)

        // Pre-fill with current goal if it exists
        val sharedPref = getSharedPreferences("UserPrefs", Context.MODE_PRIVATE)
        val currentGoal = sharedPref.getString("goal_weight", "")
        etGoalWeight.setText(currentGoal)

        btnUpdate.setOnClickListener {
            val goal = etGoalWeight.text.toString().trim()
            if (goal.isNotEmpty()) {
                with(sharedPref.edit()) {
                    putString("goal_weight", goal)
                    apply()
                }
                Toast.makeText(this, getString(R.string.goal_updated), Toast.LENGTH_SHORT).show()
                finish() // Returns to the previous screen
            } else {
                Toast.makeText(this, getString(R.string.fields_cannot_be_empty), Toast.LENGTH_SHORT).show()
            }
        }

        btnCancel.setOnClickListener {
            finish() // Returns to previous screen without saving
        }

        btnLogout.setOnClickListener {
            val intent = Intent(this, MainActivity::class.java)
            intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK
            startActivity(intent)
            finish()
        }
    }
}
