
```
# MJPEG
gst-launch-1.0 -v v4l2src device=/dev/video0 ! image/jpeg !  jpegdec ! video/x-raw,width=1920,height=1080,framerate=30/1 ! autovideosink
gst-launch-1.0 -v v4l2src device=/dev/video0 ! jpegdec ! video/x-raw,width=1920,height=1080,framerate=30/1 ! autovideosink

# YUYV
gst-launch-1.0 -v  v4l2src device="/dev/video0" ! video/x-raw,width=1920,height=1080,framerate=5/1 ! videoconvert ! ximagesink

# TO FILE
gst-launch-1.0 -v v4l2src device=/dev/video0 ! jpegdec ! video/x-raw,width=1920,height=1080,framerate=30/1 ! x264enc qp-min=18 ! avimux ! filesink location=videotestsrc.avi

# FROM UDP USERMODE
gst-launch-1.0 -v v4l2src device=/dev/video0 ! jpegdec ! video/x-raw,width=1920,height=1080,framerate=30/1 ! x264enc tune=zerolatency bitrate=1024 ! rtph264pay ! udpsink host=127.0.0.1 port=22081

# TO UDP USERMODE
gst-launch-1.0 udpsrc port=21082 caps = "application/x-rtp, media=video, clock-rate=90000, encoding-name=H264, payload=96" ! rtph264depay ! avdec_h264 ! autovideosink

# TEST FROM FILE
GST_DEBUG=qtdemux:5,avdec_h264:5 \
gst-launch-1.0 filesrc location=test.mp4 ! qtdemux name=demuxer \
    demuxer.video_0 ! h264parse ! avdec_h264 ! video/x-raw,width=3840,height=2160,framerate=30000/1001 ! autovideosink

gst-launch-1.0 filesrc location=test.mp4 ! qtdemux name=demuxer \
    demuxer.video_0 ! h264parse ! avdec_h264 ! videorate ! videoscale ! video/x-raw,width=1080,framerate=1/1 ! \
    x264enc tune=zerolatency bitrate=1024 ! rtph264pay ! udpsink host=127.0.0.1 port=22081

gst-launch-1.0 filesrc location=test_720.mp4 ! qtdemux name=demuxer demuxer.video_0 ! h264parse ! avdec_h264 ! videorate ! videoscale ! video/x-raw,framerate=30000/1001 ! x264enc tune=zerolatency bitrate=2500 ! rtph264pay ! udpsink host=127.0.0.1 port=22081

gst-launch-1.0 filesrc location=test_1080_1000kbps.mp4 ! qtdemux name=demuxer demuxer.video_0 ! h264parse ! rtph264pay ! udpsink host=127.0.0.1 port=22081
gst-launch-1.0 filesrc location=test_1080_1000kbps.mp4 ! qtdemux name=demuxer demuxer.video_0 ! rtph264pay ! rtph264depay ! avdec_h264 ! autovideosink

# save raw
gst-launch-1.0 filesrc location=test_1080.mp4 ! qtdemux name=demuxer demuxer.video_0 ! h264parse ! avdec_h264 ! filesink location=test_1080.h264
gst-launch-1.0 filesrc location=test_1080.mp4 ! qtdemux name=demuxer demuxer.video_0 ! h264parse ! avdec_h264 ! videoconvert ! video/x-raw, format=UYVY ! filesink location=test_1080.h264
gst-launch-1.0 filesrc location=test_1080.mp4 ! qtdemux name=demuxer demuxer.video_0 ! h264parse ! avdec_h264 ! videoconvert ! video/x-raw,width=1920,height=1080,framerate=30000/1001,format=UYVY ! filesink location=test_1080.h264

gst-launch-1.0 filesrc location=test_1080.h264 ! video/x-raw,width=1920,height=1080,framerate=30000/1001 ! autovideosink



```