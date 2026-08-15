package com.example.jenniferswintonweighttrackerproj2

import android.content.ContentValues
import android.content.Context
import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteOpenHelper
import java.text.SimpleDateFormat
import java.util.Locale

class DatabaseHelper(context: Context) : SQLiteOpenHelper(context, DATABASE_NAME, null, DATABASE_VERSION) {

    companion object {
        private const val DATABASE_NAME = "weight_tracker.db"
        private const val DATABASE_VERSION = 3
        private const val TABLE_NAME = "weight_entries"
        private const val COLUMN_ID = "id"
        private const val COLUMN_DATE = "date"
        private const val COLUMN_WEIGHT = "weight"
        private const val COLUMN_USER = "username"

        private const val TABLE_USERS = "users"
        private const val COLUMN_USER_ID = "user_id"
        private const val COLUMN_USERNAME = "username"
        private const val COLUMN_PASSWORD = "password"
        
        private val dateFormat = SimpleDateFormat("MM/dd/yyyy", Locale.US)
    }

    override fun onCreate(db: SQLiteDatabase) {
        val createWeightTable = ("CREATE TABLE " + TABLE_NAME + " ("
                + COLUMN_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, "
                + COLUMN_DATE + " TEXT, "
                + COLUMN_WEIGHT + " TEXT, "
                + COLUMN_USER + " TEXT" + ")")
        db.execSQL(createWeightTable)

        val createUsersTable = ("CREATE TABLE " + TABLE_USERS + " ("
                + COLUMN_USER_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, "
                + COLUMN_USERNAME + " TEXT UNIQUE, "
                + COLUMN_PASSWORD + " TEXT" + ")")
        db.execSQL(createUsersTable)
    }

    override fun onUpgrade(db: SQLiteDatabase, oldVersion: Int, newVersion: Int) {
        if (oldVersion < 2) {
            val createUsersTable = ("CREATE TABLE " + TABLE_USERS + " ("
                    + COLUMN_USER_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, "
                    + COLUMN_USERNAME + " TEXT UNIQUE, "
                    + COLUMN_PASSWORD + " TEXT" + ")")
            db.execSQL(createUsersTable)
        }
        if (oldVersion < 3) {
            db.execSQL("ALTER TABLE $TABLE_NAME ADD COLUMN $COLUMN_USER TEXT DEFAULT ''")
        }
    }

    // User management methods
    fun addUser(username: String, password: String): Long {
        val db = this.writableDatabase
        val values = ContentValues()
        values.put(COLUMN_USERNAME, username)
        values.put(COLUMN_PASSWORD, password)
        val id = db.insert(TABLE_USERS, null, values)
        db.close()
        return id
    }

    fun checkUser(username: String, password: String): Boolean {
        val db = this.readableDatabase
        val cursor = db.rawQuery("SELECT * FROM $TABLE_USERS WHERE $COLUMN_USERNAME=? AND $COLUMN_PASSWORD=?", arrayOf(username, password))
        val exists = cursor.count > 0
        cursor.close()
        db.close()
        return exists
    }

    // Weight entry methods
    fun addWeightEntry(date: String, weight: String, username: String): Long {
        val db = this.writableDatabase
        val values = ContentValues()
        values.put(COLUMN_DATE, date)
        values.put(COLUMN_WEIGHT, weight)
        values.put(COLUMN_USER, username)
        val id = db.insert(TABLE_NAME, null, values)
        db.close()
        return id
    }

    fun getAllEntries(username: String): List<WeightEntry> {
        val entries = mutableListOf<WeightEntry>()
        val db = this.readableDatabase
        val cursor = db.rawQuery("SELECT * FROM $TABLE_NAME WHERE $COLUMN_USER=?", arrayOf(username))

        if (cursor.moveToFirst()) {
            do {
                val id = cursor.getInt(cursor.getColumnIndexOrThrow(COLUMN_ID))
                val date = cursor.getString(cursor.getColumnIndexOrThrow(COLUMN_DATE))
                val weight = cursor.getString(cursor.getColumnIndexOrThrow(COLUMN_WEIGHT))
                val user = cursor.getString(cursor.getColumnIndexOrThrow(COLUMN_USER))
                entries.add(WeightEntry(id, date, weight, user))
            } while (cursor.moveToNext())
        }
        cursor.close()
        db.close()

        // Sort by date parsed from string (descending), then by ID (descending) as a fallback
        return entries.sortedWith(compareByDescending<WeightEntry> { 
            try {
                dateFormat.parse(it.date)
            } catch (e: Exception) {
                null
            }
        }.thenByDescending { it.id })
    }

    fun updateWeightEntry(id: Int, date: String, weight: String): Int {
        val db = this.writableDatabase
        val values = ContentValues()
        values.put(COLUMN_DATE, date)
        values.put(COLUMN_WEIGHT, weight)
        val result = db.update(TABLE_NAME, values, "$COLUMN_ID = ?", arrayOf(id.toString()))
        db.close()
        return result
    }

    fun deleteEntry(id: Int) {
        val db = this.writableDatabase
        db.delete(TABLE_NAME, "$COLUMN_ID = ?", arrayOf(id.toString()))
        db.close()
    }
}
