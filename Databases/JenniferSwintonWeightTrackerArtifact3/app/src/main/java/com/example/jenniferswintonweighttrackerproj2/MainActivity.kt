package com.example.jenniferswintonweighttrackerproj2

import android.content.Intent
import android.os.Bundle
import android.view.View
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.edit
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.launch

class MainActivity : AppCompatActivity() {

    private lateinit var db: AppDatabase

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_login)

        db = AppDatabase.getDatabase(this)

        val usernameInput = findViewById<EditText>(R.id.usernameInput)
        val passwordInput = findViewById<EditText>(R.id.editTextTextPassword)
        val loginButton = findViewById<Button>(R.id.loginButton)
        val createAccountButton = findViewById<Button>(R.id.createAccountButton)
        val loginErrorMessage = findViewById<TextView>(R.id.loginErrorMessage)

        loginButton.setOnClickListener {
            val username = usernameInput.text.toString().trim()
            val password = passwordInput.text.toString().trim()

            if (username.isNotEmpty() && password.isNotEmpty()) {
                lifecycleScope.launch {
                    val user = db.userDao().checkUser(username, password)
                    if (user != null) {
                        // Store the current logged-in username
                        val userPrefs = getSharedPreferences("UserPrefs", MODE_PRIVATE)
                        userPrefs.edit { putString("current_username", username) }

                        val intent = Intent(this@MainActivity, WeightTrendActivity::class.java)
                        startActivity(intent)
                        loginErrorMessage.visibility = View.GONE
                        finish() // Finish MainActivity so user can't back-navigate to Login
                    } else {
                        loginErrorMessage.visibility = View.VISIBLE
                    }
                }
            } else {
                loginErrorMessage.visibility = View.VISIBLE
            }
        }

        createAccountButton.setOnClickListener {
            val intent = Intent(this, CreateAccountActivity::class.java)
            startActivity(intent)
        }
    }
}
