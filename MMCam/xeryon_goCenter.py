from Xeryon import *
import sys
import os

def move_to_position(comPort):
    controller = Xeryon(comPort, 115200)
    axisX = controller.addAxis(Stage.XLS_1250, "X")
    controller.start()
    axisX.findIndex()

    axisX.setUnits(Units.mm)
    dpos = 0.0
    dpos = axisX.getDPOS()

    controller.stop()
    return dpos  # Return the DPOS value

def save_position_to_file(file_path, position):
    folder_path = os.path.dirname(file_path)  # Get the directory from the file path
    
    if not os.path.exists(folder_path):  # Check if the directory exists
        os.makedirs(folder_path)  # Create the directory if it doesn't exist
        
    with open(file_path, "w") as f:  # Open the file safely using 'with'
        f.write(str(position))  # Write the position to the file
    
if __name__ == "__main__":
    comPort = os.fsdecode(sys.argv[1])
    fileName = os.fsdecode(sys.argv[2])
    position = move_to_position(comPort)
    save_position_to_file(fileName, position)