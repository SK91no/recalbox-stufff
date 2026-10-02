import os
import time
from multiprocessing import Process
import RPi.GPIO as GPIO

# initialize pins

powerPin = 26
powerenPin = 27
hdmiPin = 18


# initialize GPIO settings
def init():
    GPIO.setmode(GPIO.BCM)
    GPIO.setwarnings(False)
    GPIO.setup(powerPin, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.setup(hdmiPin, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
    GPIO.setup(powerenPin, GPIO.OUT, initial=GPIO.HIGH)
    GPIO.output(powerenPin, GPIO.HIGH)


# waits for user to hold button up to 1 second before issuing poweroff command
def poweroff():
    while True:
        while GPIO.input(powerPin) == GPIO.HIGH:
            time.sleep(0.5)
        os.system("shutdown -h now")


if __name__ == "__main__":
    # initialize GPIO settings
    init()
    # create a multiprocessing.Process instance for each function to enable parallelism
    powerProcess = Process(target=poweroff)
    powerProcess.start()

    powerProcess.join()

    GPIO.cleanup()
