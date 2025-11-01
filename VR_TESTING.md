# VR Mode Testing Guide

This document provides a comprehensive testing checklist for the VR mode implementation.

## Prerequisites

- Oculus Quest (1, 2, 3, or Pro) with updated firmware
- МикроББокс robot powered on and connected to WiFi
- Robot and Quest on the same network
- Fully charged Quest controllers

## Test Environment Setup

1. **Power on the robot**
   - Verify green LED indicates WiFi connection
   - Note the robot's IP address from serial monitor or router

2. **Connect Quest to network**
   - Settings → WiFi → Connect to same network as robot
   - Verify internet connection (optional but helpful for debugging)

3. **Open Oculus Browser**
   - Launch Browser app from Quest home
   - Navigate to robot's IP address (e.g., http://192.168.1.100)

## Functional Tests

### Test 1: VR Detection and Entry

**Expected Behavior:**
- [ ] Page loads successfully with camera stream visible
- [ ] Purple "🥽 VR" button appears in top-right corner
- [ ] Button is clickable and responsive

**Steps:**
1. Load the page in Oculus Browser
2. Wait for page to fully load
3. Look for VR button in top-right
4. Click the VR button

**Success Criteria:**
- VR button appears within 2 seconds of page load
- Clicking button enters VR mode (screen goes black temporarily)
- VR session starts and displays info screen

### Test 2: VR Session Initialization

**Expected Behavior:**
- [ ] VR mode enters successfully
- [ ] Information screen displays "Режим VR активен"
- [ ] Controller status shows "Поиск контроллеров..."
- [ ] Status updates when controllers detected

**Steps:**
1. Enter VR mode
2. Wait 1-2 seconds for initialization
3. Check controller status display

**Success Criteria:**
- Session starts without errors
- UI displays properly in VR
- Controller detection messages appear

### Test 3: Controller Detection

**Expected Behavior:**
- [ ] Left controller detected automatically
- [ ] Right controller detected automatically
- [ ] Status shows "✓ Левый ✓ Правый"

**Steps:**
1. Hold both controllers
2. Press any button on each controller
3. Observe status display

**Success Criteria:**
- Both controllers detected within 5 seconds
- Status updates correctly
- No error messages in console

### Test 4: Movement - Tank Mode (Default)

**Expected Behavior:**
- [ ] Left stick controls left motors
- [ ] Right stick controls right motors
- [ ] Forward motion works (both sticks up)
- [ ] Backward motion works (both sticks down)
- [ ] Turn left works (left stick down, right stick up)
- [ ] Turn right works (right stick down, left stick up)

**Steps:**
1. Ensure tank mode is active (default)
2. Push left stick forward → left motors should run
3. Push right stick forward → right motors should run
4. Push both sticks forward → robot moves forward
5. Push both sticks back → robot moves backward
6. Push left down, right up → robot turns left
7. Push right down, left up → robot turns right

**Success Criteria:**
- Robot responds to all stick movements within 200ms
- Movement is smooth and proportional to stick position
- Releasing sticks stops the robot
- No drift when sticks are centered

### Test 5: Movement - Differential Mode

**Expected Behavior:**
- [ ] Right stick Y controls speed
- [ ] Right stick X controls turning
- [ ] Forward motion works (right stick up)
- [ ] Backward motion works (right stick down)
- [ ] Turn left works (right stick left while moving)
- [ ] Turn right works (right stick right while moving)

**Steps:**
1. Press B/Y button to switch to differential mode
2. Push right stick up → robot moves forward
3. Push right stick down → robot moves backward
4. Push right stick up+left → robot moves forward while turning left
5. Push right stick up+right → robot moves forward while turning right

**Success Criteria:**
- Mode switches successfully
- Speed control is smooth
- Turning while moving works correctly
- Releasing stick stops movement

### Test 6: Trigger - Horn/Signal

**Expected Behavior:**
- [ ] Pressing trigger activates horn
- [ ] Holding trigger keeps horn active
- [ ] Releasing trigger stops horn
- [ ] Works on both controllers

**Steps:**
1. Press and hold left trigger
2. Listen for horn sound
3. Release trigger
4. Repeat with right trigger

**Success Criteria:**
- Horn activates immediately (< 100ms)
- Sound continues while held
- Sound stops when released
- No lag or double-activation

### Test 7: Grip - Flashlight Toggle

**Expected Behavior:**
- [ ] Pressing grip toggles flashlight
- [ ] First press turns on
- [ ] Second press turns off
- [ ] Works on both controllers

**Steps:**
1. Press grip button on left controller
2. Observe camera LED (should turn on)
3. Press grip again
4. Observe LED (should turn off)
5. Repeat with right controller

**Success Criteria:**
- Toggle works reliably
- No double-toggle from single press
- State persists correctly
- Visual feedback visible

### Test 8: A/X Button - Effect Cycling

**Expected Behavior:**
- [ ] Pressing A/X cycles through effects
- [ ] Order: Normal → Police → Fire → Ambulance → Normal
- [ ] LEDs change according to mode
- [ ] Sound effects play for special modes

**Steps:**
1. Press A/X button
2. Observe LED colors (should change)
3. Listen for sound effects
4. Press A/X again
5. Repeat until all 4 modes tested

**Success Criteria:**
- Cycles through all modes correctly
- Visual effects display properly
- Sound effects match mode
- No skipped or stuck modes

### Test 9: B/Y Button - Control Mode Toggle

**Expected Behavior:**
- [ ] Pressing B/Y switches between tank and differential
- [ ] Toggle works from either mode
- [ ] Movement behavior changes immediately
- [ ] No interruption to other functions

**Steps:**
1. Start in tank mode
2. Press B/Y button
3. Test movement (should now be differential)
4. Press B/Y again
5. Test movement (should now be tank)

**Success Criteria:**
- Mode switches instantly
- Movement responds correctly in new mode
- No need to release sticks
- Toggle is reliable

### Test 10: VR Session Exit

**Expected Behavior:**
- [ ] Can exit VR using Quest button
- [ ] Can exit by pressing VR button again
- [ ] Robot stops moving on exit
- [ ] Can re-enter VR successfully

**Steps:**
1. Enter VR mode
2. Press Quest/Oculus button
3. Select "Exit VR"
4. Verify robot stops
5. Enter VR again
6. Verify everything works

**Success Criteria:**
- Clean exit without errors
- Robot stops immediately
- Can re-enter without page reload
- No lingering issues

### Test 11: Stability and Performance

**Expected Behavior:**
- [ ] No lag or stuttering in VR
- [ ] Frame rate stays smooth (60+ FPS)
- [ ] No disconnections during use
- [ ] Can operate for 5+ minutes continuously

**Steps:**
1. Enter VR mode
2. Use robot continuously for 5 minutes
3. Try various movements and features
4. Monitor performance

**Success Criteria:**
- Smooth operation throughout
- No frame drops or freezes
- Commands remain responsive
- No timeout errors

### Test 12: Edge Cases

**Expected Behavior:**
- [ ] Works after controller battery warning
- [ ] Recovers from brief WiFi interruption
- [ ] Handles rapid button presses
- [ ] Deals with stick at max deflection

**Steps:**
1. Test with low controller battery (< 20%)
2. Briefly disconnect WiFi, then reconnect
3. Press buttons rapidly (spam test)
4. Hold stick at maximum deflection

**Success Criteria:**
- No crashes or errors
- Graceful handling of issues
- Recovery is automatic
- User is informed of problems

## Issues to Watch For

### Critical Issues (Must Fix)
- [ ] VR button doesn't appear
- [ ] Cannot enter VR mode
- [ ] Controllers not detected
- [ ] No movement response
- [ ] Crashes or freezes

### High Priority Issues
- [ ] Significant lag (> 500ms)
- [ ] Frequent disconnections
- [ ] Incorrect button mappings
- [ ] Movement in wrong direction

### Medium Priority Issues
- [ ] Minor lag (200-500ms)
- [ ] Occasional missed inputs
- [ ] UI elements misaligned
- [ ] Console errors (non-breaking)

### Low Priority Issues
- [ ] Small UI inconsistencies
- [ ] Minor visual glitches
- [ ] Rare edge case bugs

## Debug Information Collection

If issues occur, collect:

1. **Browser Console Log**
   - Press F12 in Oculus Browser (if available)
   - Or use remote debugging from PC

2. **Network Info**
   - WiFi signal strength
   - Ping time to robot
   - Any timeout errors

3. **Controller Info**
   - Battery levels
   - Firmware version
   - Pairing status

4. **Robot Info**
   - Serial monitor output
   - WiFi connection status
   - Firmware version

## Success Metrics

The VR implementation is considered successful if:
- ✅ All 12 functional tests pass
- ✅ No critical or high priority issues
- ✅ Less than 3 medium priority issues
- ✅ Response time < 200ms
- ✅ Can operate for 10+ minutes without issues

## Notes

- Test in good WiFi conditions (< 5m from router)
- Ensure robot has adequate power
- Use freshly charged Quest controllers
- Test from a safe location for both robot and user
