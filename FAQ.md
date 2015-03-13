OpenDCP FAQ Version 1.1

**What is OpenDCP?**
> OpenDCP is a tool to create digital cinema packages for use in today's digital cinemas.

**What systems does OpenDCP run on?**
> The GUI/CLI versions are pre-compiled for Macintosh OSX 10.6, Windows and Linux. It should be able to run on any posix-type OS.

**What type of source content is required?**
> OpenDCP currently takes content from the Digital Cinema Distribution Master (DCDM) space, which is:
    1. 8/12/16-bit RGB Tiff Images (technically only 16-bit is DCDM, but OpenDCP accepts more)
    1. 24-bit PCM 48khz or 96khz wav files (each channel is a separate mono wav file)

**What does OpenDCP output?**
> OpenDCP will convert Tiff or DPX image sequences to Jpeg2000 images and convert them to the XYZ colorspace. It will create the necessary MXF files and XML files for the DCP.

**Is MPEG2 supported?**
> OpenDCP will create MPEG2 DCPs, but this is no longer a DCI compliant format and should be avoided in most cases.

**Does OpenDCP create MXF Interop or SMPTE packages?**
> It will create both types.

**Does OpenDCP support Stereoscopic (3D)?**
> Yes, you can create stereoscopic DCPs.

**What frame rates are supported?**
> The major frame rates - 24, 25, 30, 48, 50, 60 in 2D and 24, 48 in 3D are supported. Not all servers support all frame rates, especially with MXF Interop packages. So, the safest choice is to stick with 24fps.

**Does OpenDCP support KDMs?**
> No, it is a planned feature, but there is no timeline for it to be implemented.

**Does OpenDCP support XXX files types?**
> TIFF and DPX are supported

**Why can't OpenDCP just take a video file, like mp4 or Apple ProRes?**
> There are a couple reasons. OpenDCP was originally developed to solve missing pieces in the digital cinema workflow, creating a DCP, which includes DCI compliant JPEG2000 encoding, XYZ color space conversion, MXF wrapping, and XML generation. Many videos are natively 23.97 or 29.97 fps and converting those to 24fps or 30fps requires a little more involvement by the user to ensure audio is properly synced. That is something you want to make sure is right from the start, not after spending hours encoding only to find out it didn't come out right. There are already dozens of ways to readily export a video to a TIFF image sequence. Additionally, there may be licensing issues with mp4 and other codecs. This may be added in the future, but it is not a priority at the moment.

**Why are the colors are all weird (washed out/green/desaturated/etc) after the  XYZ conversion?**
> The colors are in the XYZ colorspace, which can't be viewed on a standard RGB monitor as it will not know how to decode the image.

**Why are my images 8-bit when I open them in application X? They should be 12-bit?**

Most applications only understand 8/16/32 bit images. This causes a 12-bit image to appear as an 8-bit.

**The audio sounds distorted, like its skipping or clicking, or the pitch is down, what's wrong?**
> Most likely you didn't supply a 1 channel mono wav file.

**The audio and video MXF files have a difference of 1 frame, what's wrong?**
> This happens because audio and video are not frame aligned and one was longer than the other by 1/24 of second (assuming 24fps). This is pretty common and not an issue, just adjust the duration during the DCP creation.

**What servers does OpenDCP work with?**
> OpenDCP has been tested against the Sony, DoRemi, GDC, and Dolby servers.

**Is OpenDCP really free?**
> OpenDCP is open source and currently free.

**Wait, does that mean you are going to charge for it eventually?**
> An advanced or professional version may be released for a fee to cover the costs of licensing commercial components. This would be for something like faster Jpeg2000 encoding.