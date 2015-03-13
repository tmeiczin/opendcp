# Introduction #
OpenDCP is a software tool to create Digital Cinema Packages (DCPs) for use in digital theaters.

# Source Material #
In order to create a DCP, you need to provide source material in a form that OpenDCP can use, which is typically the Digital Content Distribution Master (DCDM):

  * 8/16/24-bit TIFF image sequence (DPX can also be used)
  * 48/96khz 24-bit PCM Mono (1 channel) Wav files.

There are various ways, such as FFmpeg, Quicktime, or Final Cut that you can create TIFF image sequence and multiple 1 channel wav files. When exporting your image sequence it is important to use padded file names. All filenames need to be the same length and share the base component.

For example:

  * image-0001.tiff
  * image-0002.tiff
  * ...
  * image-0100.tiff

**Do NOT** do something like :

  * image-1.tiff
  * image-2.tiff
  * image-100.tiff


**Do NOT** do something like :
  * image-1.tiff
  * anotherimage-2.tiff

**IMPORTANT: Your file names and paths should contain only US ASCII characters**

# JPEG2000 Conversion #
Once you have your source image sequence in TIFF or DPX format, you'll need to perform the JPEG2000 conversion.

In the GUI, select the "JPEG2000" tab. You'll be presented with a number of options, but most of them are self descriptive.

![http://opendcp.googlecode.com/files/jpeg2000.png](http://opendcp.googlecode.com/files/jpeg2000.png)

#### JPEG2000 Encoder Parameters ####

  * **Encoder**: This is the JPEG2000 encoder. On most systems only OpenJPEG will be listed.
  * **Overwrite Existing**: If this selected, existing JPEG2000 files will be re-encoded and overwritten. Uncheck if are resuming and it will only encoded files not present in the destination directory.
  * **Profile**: This is the destination DCI profile, either 2K or 4K.
  * **Stereoscopic**: Select this if you are creating a 3D sequence.
  * **Frame Rate**: This is the frame rate of your source. This is only used to calculate image sizes, it does not  rate convert. It is important you set this to your expected frame rate.
  * **Bandwidth**: This sets the overall bit rate of your image sequence. A higher value doesn't always mean higher quality. For 2K@24fps, 125 mb/s is more than enough and 75mb/s is generally recommended.
  * **Threads**: The number of threads should be auto-detected, but if you have limited memory or want to limit the CPU usage, you can adjust the number of threads.

#### Image Parameters ####
  * **Source Color**: The source image color profile.
  * **XYZ Conversion**: Select this to perform the XYZ conversion. **YES, IT IS NORMAL** for your images to look strange after. You can't properly view XYZ images on your RGB monitor.
  * **DPX Logarithmic**: If your source images are DPX, select this if they are logarithmic.
  * **DCI Resize**: If your image resolutions are not DCI compliant, select a scaling algorithm and they will be scaled to fit the nearest DCI resolution. It is recommended you do this in your editing package instead for better quality.

#### Directories ####
  * Select the directory where your source images are (if stereoscopic select the left and right directories respectively).
  * Select the destination directory where the JPEG2000 images will be saved (again, if stereoscopic select left and right directories).

#### Convert ####
  * Click convert and the process will begin. Be aware this is a **slow** process. Even on a high end system you might only get 2-3fps.

# MXF #
Once you have JPEG2000 images and mono wav files, we now create MXF files. The MXF file is basically a bundle of all your JPEG2000 or wav files and some meta data. This allows all those files to be managed as a single file. Under the MXF tab, you'll see some options.

![http://opendcp.googlecode.com/files/mxf.png](http://opendcp.googlecode.com/files/mxf.png)

#### MXF Parameters ####
  * **Source**: This is the type of content you are using to create the MXF. For pictures MXF tracks you can select JPEG2000 or MPEG2. For audio tracks you'll select WAV
  * **Package**: This is the label set for you MXF file. SMPTE is the newest specification and MXF Interop is deprecated. SMPTE is now the preferred type, but you should check with the destination venue to be sure.
  * **Frame Rate**: Select the frame rate of your source material.

#### Picture Parameters ####
This section appears when JPEG2000 or MPEG2 is selected as the source. These options vary depending the chosen source

  * **Stereoscopic**: Select this if you are making a stereoscopic package.
  * **Slideshow**: If you want to make a slideshow or a still picture, check this tab. This will take a JPEG2000 image and create a slide MXF of the specified duration. If you select a directory with multiple images, then it will create a slideshow from those images. Each slide will appear for the specified duration.

#### Sound Parameters ####
Here you select the options for your sound MXF file

  * **Stereo**: Click if you want to create a 2 channel stereo MXF.
  * **5.1**: Click if you want to create a 5.1 MXF
  * **7.1**: Click if you are creating a 7.1 MXF
  * **Hearing/Visually Impaired**: Select if adding HI/VI support

#### Directories ####
The number of directories will vary depending on your source and selections, but it is pretty self explanatory. Select source files and destinations.

#### Create ####
Once all the options are set, click create and your MXF files will be created.

# DCP #
This is the final step in creating the DCP. It takes the MXF files and generates XML data that describes them to the digital server what they are and how they are to be played.

Here you add your previously created MXF files to form reels. When you add the an MXF, OpenDCP will show the duration of that item. You need to make sure the duration of all MXF files is the same. In some cases, your audio and video MXF files may differ by a couple frames. You can adjust their durations to match. This will not change the source MXF, it only indicates to digital server to when to start/stop for a give MXF.

![http://opendcp.googlecode.com/files/dcp.png](http://opendcp.googlecode.com/files/dcp.png)