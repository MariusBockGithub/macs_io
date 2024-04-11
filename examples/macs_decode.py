import argparse
import sys
from matplotlib import pyplot as plt
# - register command line parameters
parser = argparse.ArgumentParser(description='Macs Info Python Script')
parser.add_argument('-i', '--image', action='store', required=True)
parser.add_argument('-p', '--path-to-macs-io-python', action='store', default="")
parser.add_argument('--path-to-macs-io-hu', action='store', default="")
parser.add_argument('--gamma', action='store', default=1.)
parser.add_argument('--stretch-min', action='store', default=0.)
parser.add_argument('--stretch-max', action='store', default=1.)
parser.add_argument('--to-8bit', action='store_true', default=False)
parser.add_argument('--color-balance-r', action='store', default=1.)
parser.add_argument('--color-balance-g', action='store', default=1.)
parser.add_argument('--color-balance-b', action='store', default=1.)
parser.add_argument('--distortion-cx-px', action='store', default=-1.)
parser.add_argument('--distortion-cy-px', action='store', default=-1.)
parser.add_argument('--distortion-k1', action='store', default=0.)
parser.add_argument('--distortion-k2', action='store', default=0.)
parser.add_argument('--distortion-k3', action='store', default=0.)
parser.add_argument('--devignetting-a', action='store', default=0.)
parser.add_argument('--devignetting-b', action='store', default=0.)
parser.add_argument('--devignetting-c', action='store', default=0.)
parser.add_argument('--devignetting-cx', action='store', default=0.)
parser.add_argument('--devignetting-cy', action='store', default=0.)
parser.add_argument('--devignetting-factor', action='store', default=0.)
parser.add_argument('--devignetting-offset', action='store', default=0)

# - load macs_io_python
args = parser.parse_args()
sys.path.append(args.path_to_macs_io_python)
import macs_io_python

# - fill the corrections options
options = macs_io_python.CorrectionOptions()
options.stretch.gamma = args.gamma
options.stretch.min = args.stretch_min
options.stretch.max = args.stretch_max
options.color_balance.r = args.color_balance_r
options.color_balance.g = args.color_balance_g
options.color_balance.b = args.color_balance_b
options.convertTo8bit = args.to_8bit
options.devignetting.a = args.devignetting_a
options.devignetting.b = args.devignetting_b
options.devignetting.c = args.devignetting_c
options.devignetting.cx = args.devignetting_cx
options.devignetting.cy = args.devignetting_cy
options.devignetting.factor = args.devignetting_factor
options.devignetting.offset = args.devignetting_offset
options.distortion.cx_px = args.distortion_cx_px
options.distortion.cy_px = args.distortion_cy_px
options.distortion.k1 = args.distortion_k1
options.distortion.k2 = args.distortion_k2
options.distortion.k3 = args.distortion_k3

# - load the image
macsImage = macs_io_python.MacsImage()
if not macsImage.load(args.image):
    print(f"Could not load image {args.image}")
    exit()
print

# - the corrected image is always 8 or 16 bit (debayered) image
# - this is the preferred way of interacting with MACS images
# - if you really want the raw data you can use macsImage.imageData.rawData()
# - be careful and take the macsImage.imageData.format() and macsImage.imageData.endianness() into account
# - use macsImage.imageData.data() to get 16 bit little endian data
corrected = macsImage.getCorrectedImage(options)
if not args.to_8bit:
    corrected = (corrected/256).astype('uint8')
plt.imshow(corrected, interpolation='nearest')
plt.show()





