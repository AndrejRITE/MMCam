from Xeryon import *
import sys

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