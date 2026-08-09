package com.example.jenniferswintonweighttrackerproj2

import android.os.Parcelable
import kotlinx.parcelize.Parcelize

@Parcelize
data class WeightEntry(
    val id: Int,
    val date: String,
    val weight: String,
    val username: String = ""
) : Parcelable