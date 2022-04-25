# ESPAmbient
ESP32 NeoPixel(WS2813) + iOS Control App.

# Features
 - Color control with included iOS Application.(SwiftUI, needs Xcode, iOS 15.0+)
 - Brightness Control
 - It supports 3 Types of color.(Welcome Light, United Color, Gradient(2 Colors))
 - Sent data from Application is stored in ESP32.

## How It Works?
It receives data from iOS App. and displays on the NeoPixel Strip

## Issues
I don't know why, but ESP32 burns. → resolved by adding 1000uF Capacitor.
Data is not syncronized with application. 


## To be...
 - Data Synchronization
 - Device Identification.

## Referenced & Based on
 - Cuvenx(https://www.cuvenx.com/post/ble-peripheral-ios-development-part1)
 - FastLED

## Changelog
 - 1.0.1 Stable(discard of useless features.(AccSync, 4-color Gradient)