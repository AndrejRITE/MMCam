from Xeryon import *
import sys
import os

def stop_and_report_position(comPort):
    controller = Xeryon(comPort, 115200)
    axisX = controller.addAxis(Stage.XLS_1250, "X")
    controller.start()

    axisX.setUnits(Units.mm)

    # Real, immediate halt: sends STOP=0 straight to the controller.
    # Unlike controller.stop(), this does NOT send the stage home (ZERO=0)
    # and does NOT close communication - it just stops the motion where it
    # currently is.
    controller.stopMovements()

    # Read back the true position now that the stage has actually stopped,
    # so the caller isn't left trusting a stale/intended-target value.
    dpos = axisX.getDPOS()
    return dpos

def save_position_to_file(file_path, position):
    folder_path = os.path.dirname(file_path)  # Get the directory from the file path

    if not os.path.exists(folder_path):  # Check if the directory exists
        os.makedirs(folder_path)  # Create the directory if it doesn't exist

    with open(file_path, "w") as f:  # Open the file safely using 'with'
        f.write(str(position))  # Write the position to the file

if __name__ == "__main__":
    comPort = os.fsdecode(sys.argv[1])
    fileName = os.fsdecode(sys.argv[2])
    position = stop_and_report_position(comPort)
    save_position_to_file(fileName, position)
