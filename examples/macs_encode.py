import argparse
import datetime
import sys
import time

import numpy

# - register command line parameters
parser = argparse.ArgumentParser(description='Macs Info Python Script')
parser.add_argument('-o', '--output', action='store', required=True)
parser.add_argument('-p', '--path-to-macs-io-python', action='store', default="")

# - load macs_io_python
args = parser.parse_args()
sys.path.append(args.path_to_macs_io_python)
import macs_io_python


# - load the image
macsImage = macs_io_python.MacsImage()

macsImage.metaData.camVendor = "DLR-OS";
macsImage.metaData.camModel = "hr16070MFLGEC";
macsImage.metaData.camName = "Cam-NIR-50";
macsImage.metaData.camSerial = "33552";
macsImage.metaData.camMAC = "ac:4f:fc:00:51:f4";
macsImage.metaData.camIP = "192.168.101.236";
macsImage.metaData.camFirmware = "1.6.5";
macsImage.metaData.imageID = 2135;
macsImage.metaData.imageIDX = 2135;
macsImage.metaData.tapCount = 4;
macsImage.metaData.expTimeUS = 600;
macsImage.metaData.timeStamp = 12466170;

macsImage.geoPose.time = datetime.datetime.now()
macsImage.geoPose.roll = 0.87579313184499341;
macsImage.geoPose.pitch = 2.5735878935747571 ;
macsImage.geoPose.yaw = 23.812077442224226;
macsImage.geoPose.lat = 69.517744664608387;
macsImage.geoPose.lon = -139.09295924255881;
macsImage.geoPose.alt = 391.84334826655686;
macsImage.geoPose.veln = 57.103192015934745;
macsImage.geoPose.vele = 28.053951739469827;
macsImage.geoPose.velup = -0.17086418721157415;

data = numpy.array([127, 128, 129, 130], dtype=numpy.int16)
macsImage.imageData.init(data, 2, 2, 2, macs_io_python.Mono16, macs_io_python.Little)
if not macsImage.save(args.output, False, False):
    print("Could not save image")

