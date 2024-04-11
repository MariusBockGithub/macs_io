#include "macs_io.h"
#include <QCoreApplication>

int main(int argc, char*argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();
    
    // options
    MACS::CorrectionOptions options;

    for (int i = 0; i < args.size(); i++)
    {
        if (args[i].contains('='))
        {
            QStringList pair = args[i].split('=');
            QString key = pair.front();
            QString value = pair.back();
            args.removeAt(i--);

            if (key == "stretch_min")
                options.stretch.min = value.toDouble();
            else if (key == "stretch_max")
                options.stretch.max = value.toDouble();
            else if (key == "gamma")
                options.stretch.gamma = value.toDouble();
            else if (key == "devignette_a")
                options.devignetting.a = value.toDouble();
            else if (key == "devignette_b")
                options.devignetting.b = value.toDouble();
            else if (key == "devignette_c")
                options.devignetting.c = value.toDouble();
            else if (key == "devignette_offset")
                options.devignetting.offset = value.toInt();
            else if (key == "devignette_factor")
                options.devignetting.factor = value.toDouble();
            else if (key == "color_balance_r")
                options.color_balance.r = value.toDouble();
            else if (key == "color_balance_g")
                options.color_balance.g = value.toDouble();
            else if (key == "color_balance_b")
                options.color_balance.b = value.toDouble();
            else if (key == "cx")
                options.distortion.cx_px = value.toDouble();
            else if (key == "cy")
                options.distortion.cy_px = value.toDouble();
            else if (key == "k1")
                options.distortion.k1 = value.toDouble();
            else if (key == "k2")
                options.distortion.k2 = value.toDouble();
            else if (key == "k3")
                options.distortion.k3 = value.toDouble();
            else if (key == "bitDepth")
            {
                if (value.trimmed() == "8")
                    options.convertTo8bit = true;
                else if (value.trimmed() == "16")
                    options.convertTo8bit = false;
                else
                {
                    qDebug() << "Invalid value for bitDepth";
                    return -1;
                }
            }
            else
                qDebug() << "Warning: Unknown parameter:" << key;
        }
    }

    // Sanity check.
    if (args.size() != 3)
    {
        qDebug() << "Usage:\n\tmacs_decode [options] in.macs out.jpg";
        qDebug() << "where options are:";
        
        qDebug() << "\nStretch:";
        qDebug() << "\tstretch_min=0.0           stretch factor min";
        qDebug() << "\tstretch_max=1.0           stretch factor max";
        qDebug() << "\tgamma=1.0                 gamma correction";
        
        qDebug() << "\nDevignetting:";
        qDebug() << "\tdevignette_a=0.0          model based devignetting parameter";
        qDebug() << "\tdevignette_b=0.0          model based devignetting parameter";
        qDebug() << "\tdevignette_c=0.0          model based devignetting parameter";
        qDebug() << "\tdevignette_offset=0       model based devignetting parameter";
        qDebug() << "\tdevignette_factor=1.0     model based devignetting parameter";
        
        qDebug() << "\nColor balance:";
        qDebug() << "\tcolor_balance_r=1.0       color balance parameter";
        qDebug() << "\tcolor_balance_g=1.0       color balance parameter";
        qDebug() << "\tcolor_balance_b=1.0       color balance parameter";

        qDebug() << "\nDistortion:";
        qDebug() << "\tcx=w/2                    principal point";
        qDebug() << "\tcy=h/2                    principal point";
        qDebug() << "\tk1=0.0                    radial lens distortion parameter";
        qDebug() << "\tk2=0.0                    radial lens distortion parameter";
        qDebug() << "\tk3=0.0                    radial lens distortion parameter";
        return -1;
    }
    if (!options.convertTo8bit)
    {
        if (!args[2].toLower().endsWith(".tif") && !args[2].toLower().endsWith(".tiff"))
        {
            qDebug() << "Only tiff images supported as output in 16 bit mode.";
            return -1;
        }
    }
    
    const QString inputFileName = args[1];
    const QString outputFileName = args[2];

    // Load the image.
    MACS::MacsImage image;
    if (!image.load(args[1]))
    {
        return -1;
    }
    if (options.distortion.cx_px<0)
        options.distortion.cx_px = 0.5*image.imageData.width();
    if (options.distortion.cy_px<0)
        options.distortion.cy_px = 0.5*image.imageData.height();
    
    // Save the image.
    if (!image.exportImage(outputFileName, options))
    {
        qDebug() << "Couldn't write to" << args[2];
        return -1;
    }
    return 0;
}
