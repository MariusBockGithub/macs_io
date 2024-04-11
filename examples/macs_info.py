import argparse
import sys
def getIndent(count):
    return ' ' * count
parser = argparse.ArgumentParser(description='Macs Info Python Script')
parser.add_argument('-i', '--images', action='store', required=True, nargs="+")
parser.add_argument('-p', '--path-to-macs-io-python', action='store', default="")
args = parser.parse_args()
sys.path.append(args.path_to_macs_io_python)
import macs_io_python
for imageName in args.images:
    img = macs_io_python.MacsImage()
    if not img.load(imageName):
        print(f"Could not load image {imageName}")
        continue
    print
    print(f"#### MascImageInfo: {imageName}")
    indent = 4;
    ljust = 20;
    print(f"{getIndent(indent)}MascImage.ImageData")
    indent = 8;
    print(f"{getIndent(indent)}{"width:".ljust(ljust)} {img.imageData.width()}")
    print(f"{getIndent(indent)}{"height:".ljust(ljust)} {img.imageData.height()}")
    print(f"{getIndent(indent)}{"pitch:".ljust(ljust)} {img.imageData.pitch()}")
    print(f"{getIndent(indent)}{"bitDepth:".ljust(ljust)} {img.imageData.bitDepth()}")
    print(f"{getIndent(indent)}{"format:".ljust(ljust)} {img.imageData.format()}")
    print(f"{getIndent(indent)}{"endianness:".ljust(ljust)} {img.imageData.endianness()}")
    print(f"{getIndent(indent)}{"isColor:".ljust(ljust)} {img.imageData.isColor()}")
    print(f"{getIndent(indent)}{"isMono:".ljust(ljust)} {img.imageData.isMono()}")
    print(f"{getIndent(indent)}{"isValid:".ljust(ljust)} {img.imageData.isValid()}")
    print(f"{getIndent(indent)}{"16 bit data size:".ljust(ljust)} {len(img.imageData.data())}")
    print(f"{getIndent(indent)}{"raw data size:".ljust(ljust)} {len(img.imageData.rawData())}")
    indent = 4;
    ljust = 20;
    print(f"{getIndent(indent)}MascImage.GeoPose")
    indent = 8;
    print(f"{getIndent(indent)}{"time:".ljust(ljust)} {img.geoPose.time}")
    print(f"{getIndent(indent)}{"alt:".ljust(ljust)} {img.geoPose.alt}")
    print(f"{getIndent(indent)}{"lat:".ljust(ljust)} {img.geoPose.lat}")
    print(f"{getIndent(indent)}{"lon:".ljust(ljust)} {img.geoPose.lon}")
    print(f"{getIndent(indent)}{"pitch:".ljust(ljust)} {img.geoPose.pitch}")
    print(f"{getIndent(indent)}{"yaw:".ljust(ljust)} {img.geoPose.yaw}")
    print(f"{getIndent(indent)}{"roll:".ljust(ljust)} {img.geoPose.roll}")
    print(f"{getIndent(indent)}{"veln:".ljust(ljust)} {img.geoPose.veln}")
    print(f"{getIndent(indent)}{"vele: ".ljust(ljust)} {img.geoPose.vele}")
    print(f"{getIndent(indent)}{"velup:".ljust(ljust)} {img.geoPose.velup}")
    indent = 4;
    ljust = 20;
    print(f"{getIndent(indent)}MascImage.MetaData")
    indent = 8;
    print(f"{getIndent(indent)}{"affix:".ljust(ljust)} {img.metaData.affix}")
    print(f"{getIndent(indent)}{"camFirmware:".ljust(ljust)} {img.metaData.camFirmware}")
    print(f"{getIndent(indent)}{"camIP:".ljust(ljust)} {img.metaData.camIP}")
    print(f"{getIndent(indent)}{"camMAC:".ljust(ljust)} {img.metaData.camMAC}")
    print(f"{getIndent(indent)}{"camModel:".ljust(ljust)} {img.metaData.camModel}")
    print(f"{getIndent(indent)}{"camName:".ljust(ljust)} {img.metaData.camName}")
    print(f"{getIndent(indent)}{"camSerial:".ljust(ljust)} {img.metaData.camSerial}")
    print(f"{getIndent(indent)}{"camVendor:".ljust(ljust)} {img.metaData.camVendor}")
    print(f"{getIndent(indent)}{"comment:".ljust(ljust)} {img.metaData.comment}")
    print(f"{getIndent(indent)}{"expTimeUS:".ljust(ljust)} {img.metaData.expTimeUS}")
    print(f"{getIndent(indent)}{"imageID:".ljust(ljust)} {img.metaData.imageID}")
    print(f"{getIndent(indent)}{"imageIDX:".ljust(ljust)} {img.metaData.imageIDX}")
    print(f"{getIndent(indent)}{"tapCount:".ljust(ljust)} {img.metaData.tapCount}")



