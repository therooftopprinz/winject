
```
# MJPEG
gst-launch-1.0 -v v4l2src device=/dev/video0 ! image/jpeg,width=1920,height=1080,videorate=30/1 ! rtpjpegpay !  udpsink host=192.168.2.106 port=12345
gst-launch-1.0 udpsrc port=12345 caps = "application/x-rtp, media=video, payload=96" ! rtpjpegdepay ! jpegdec ! autovideosink

# YUYV
gst-launch-1.0 -v  v4l2src device="/dev/video0" ! video/x-raw,width=1920,height=1080,framerate=5/1 ! videoconvert ! ximagesink

# TO FILE
gst-launch-1.0 -v v4l2src device=/dev/video0 ! jpegdec ! video/x-raw,width=1920,height=1080,framerate=30/1 ! x264enc qp-min=18 ! avimux ! filesink location=videotestsrc.avi

# FROM UDP USERMODE
gst-launch-1.0 -v v4l2src device=/dev/video0 ! jpegdec ! video/x-raw,width=1920,height=1080,framerate=30/1 ! x264enc tune=zerolatency bitrate=1024 ! rtph264pay ! udpsink host=127.0.0.1 port=22081

gst-launch-1.0 -v v4l2src device=/dev/video0 ! image/jpeg,width=1920,height=1080,videorate=30000/1 ! jpegdec ! mpph265enc ! video/x-h265,profile=high ! rtph265pay config-interval=1 ! udpsink auto-multicast=true host=224.1.1.1 port=12345


# TO UDP USERMODE
gst-launch-1.0 udpsrc port=21082 caps = "application/x-rtp" ! rtph264depay ! avdec_h264 ! clockoverlay time-format="%F %H:%M:%S" ! autovideosink

gst-launch-1.0 udpsrc multicast-group=224.1.1.1 auto-multicast=true port=12345 caps = "application/x-rtp, media=video, payload=96" ! rtph265depay ! avdec_h265 ! autovideosink

gst-launch-1.0 udpsrc multicast-group=224.1.1.1 auto-multicast=true port=12345 caps = "application/x-rtp, media=video, payload=96" ! rtph265depay ! h265parse ! mp4mux ! filesink location=file.mp4

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

gst-launch-1.0 -v v4l2src device=/dev/video0 ! image/jpeg,width=1920,height=1080,videorate=30000/1 ! jpegdec ! clockoverlay time-format="%D %H:%M:%S" ! mpph265enc ! video/x-h265,profile=high ! rtph265pay config-interval=1 ! udpsink host=192.168.2.106 port=12345


H264

gst-launch-1.0 -v v4l2src device=/dev/video4 ! jpegdec ! video/x-raw,width=1280,height=720,framerate=30/1 ! x264enc tune=zerolatency bitrate=1000 ! rtph264pay ! udpsink host=192.168.2.144 port=22071 

gst-launch-1.0 udpsrc port=21072 caps = "application/x-rtp, media=video, clock-rate=90000, encoding-name=H264, payload=96" ! rtph264depay ! avdec_h264 ! autovideosink

```