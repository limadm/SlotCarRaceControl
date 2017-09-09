System interface:
=================

	- Two Tracks, relay-switched to 24VDC power supply;
	- Lightning LED strips, relay-switched to 24VDC;
	- 16x2 Character Display, Race Status;
	- Four Buttons:
		- POWER  - Power to Track
		- LIGHTS - Power light circuit
		- STOP/DOWN - Lap Down / Race Stop
		- START/UP  - Lap Up / Race Start
	- Two IR photodiodes sense the finish line, one per track;
	- Three semaphore LEDs (red/red/green) (optional);
	- Piezoelectric buzzer (optional).



System state diagram:
=====================

After initial power on, arduino boots then follows the phases below:

	┌────────────────┐
	│ 1.Race Setup   │←┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┬┄┄┄┄┄┄┄┄┄┄┄┐
	└────────────────┘                          ↑           ┆
	        ┆                                   ┆           ┆
	      START                            POWER/START      ┆
	        ┆                                   ┆           ┆
	        ↓                                   ┆           ┆
	┌────────────────┐                 ┌─────────────────┐  ┆
	│                │                 │                 │  ┆
	│ 2.Race Running │┄┄┄┄┄┄┄LAST LAP┄→│ 4.Race Finished │  ┆
	│                │←┄┄┄┄┐           │                 │  ┆
	└────────────────┘     ┆           └─────────────────┘  ┆
	        ┆              ┆                                ┆
	   POWER/STOP          ┆                                ┆
	        ┆            START                              ┆
	        ↓              ┆                                ┆
	┌────────────────┐     ┆                                ┆
	│ 3.Race Paused  │┅┅┅┅┅┴┄┄┄┄┄┄┄┄┄┄STOP┄→┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┘
	└────────────────┘


1. Race Setup
-------------

During race setup, configure number of laps (buttons STOP/DOWN and START/UP)
and turn lights (LIGHTS) and power (POWER) on. The system outputs
this information in the 16x2 character display as below:

	 ├┄┄┄16 chars┄┄┄┤
	┌────────────────┐
	│Leds:ON  Laps:15│ ←┄┄┄ lighting power / number of laps
	│Cars:OFF  +23.8V│ ←┄┄┄ track power    / supply voltage
	└────────────────┘

When the POWER is on and START/UP is pressed, the semaphore turns off
and a three second countdown happens. At each second, the buzzer will sound
for half a second, and the semaphore is turned on one light per second.

At the third level, the green semaphore LED lights up,
each track chronometer starts, and Phase 2 (Race Running) begins.

Players should then accelerate!

If the Finish Line Sensor detects a car before the green light (false start),
the system is RESET and goes back to Race Setup.


2. Race Running
---------------

	┌────────────────┐
	│1-04/15 MM'SS"cs│
	│2-03/15 MM'SS"cs│
	└────────────────┘
	 ↑  ↑      ↑
	 ┆ lap    best
	 ┆
	 └┄┄ lane

The first finish line is not recorded (because the cars are just before the finish line).
Then, when each Finish Line Sensor senses a passing car, the counter for that track is increased
and the chronometer is stopped (thus displayed in the "best lap" screen, if it is the best lap)
then reset and started again for the next lap. When the first car completes the number of laps, the race is finished and the system goes to Phase 4
(Race Finished).

Also, the race can be paused at any time if:

	- The track POWER is shut off;
	- The STOP/DOWN button is pressed;

so the system is put in Phase 3 (Race Paused).


3. Race Paused
--------------

	┌────────────────┐
	│1-04/15 - PAUSED│
	│2-03/15   +23.8V│
	└────────────────┘

When the race is paused, the track power is automatically shut off, so
any car or track accident may be cleared and cars may be repositioned.

When the START/UP button is pressed, the race continues to Phase 2 (Race Running).
The lap counter and chronometer will NOT be reset, so any disputes should be resolved
by putting the car in the appropriate position of the track, including any handicap delay,
if appropriate.

If the STOP/DOWN button is pressed, the system will RESET all counters, chronometers and
will go back to Phase 1 (Race Setup). Use this to cancel the current race and start a new one.


4. Race Finished
----------------

	┌────────────────┐
	│- WINNER is 1! -│
	│Best:1 MM'SS"mss│
	└────────────────┘

When a race runs until one of the cars finishes its final lap, all counters and chronometer is
paused, this track is announced as winner, and its best lap will show on the display.

In the case the two cars finish in the exact millisecond, a TIE will be shown, the best lap
of the race will be shown!

	┌────────────────┐
	│----- TIE! -----│
	│Best:1 MM'SS"mss│
	└────────────────┘

The system is automatically RESET 60 seconds after the race is finished.
