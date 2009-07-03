TRACKING = 'tracking'

LOC_PANS = 'locPans'

PAN_LEFT_ONCE = 'panLeftOnce'

MAX_PAN_SPEED = 90              # deg/sec

TRACKER_FRAMES_ON_TRACK_THRESH = 1
TRACKER_FRAMES_OFF_REFIND_THRESH = 25
ACTIVE_LOC_STARE_THRESH = 45
ACTIVE_LOC_OFF_REFIND_THRESH = 40

NUM_ACTIVE_PANS = 2
(PAN_LEFT,
 PAN_RIGHT) = range(NUM_ACTIVE_PANS)

PAN_UP_PITCH_THRESH = 10

NUM_LOOK_DIRS = 4
(LOOK_LEFT,
 LOOK_UP,
 LOOK_RIGHT,
 LOOK_DOWN) = range(NUM_LOOK_DIRS)
