from Xeryon import *
import sys

def move_to_position(comPort, absolutePosition):
    controller = Xeryon(comPort, 115200)
    axisX = controller.addAxis(Stage.XLS_1250, "X")
    controller.start()

    axisX.setUnits(Units.mm)
    axisX.setDPOS(absolutePosition)
    dpos = axisX.getDPOS()

    controller.stop()
    return dpos  # Return the DPOS value