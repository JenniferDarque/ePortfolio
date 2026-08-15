package com.example.jenniferswintonweighttrackerproj2

import androidx.room.Dao
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import androidx.room.Update

@Dao
interface WeightEntryDao {
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun addWeightEntry(entry: WeightEntry): Long

    @Query("SELECT * FROM weight_entries WHERE username = :username")
    suspend fun getAllEntries(username: String): List<WeightEntry>

    @Update
    suspend fun updateWeightEntry(entry: WeightEntry): Int

    @Query("DELETE FROM weight_entries WHERE id = :id")
    suspend fun deleteEntry(id: Int)
}
