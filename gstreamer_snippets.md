
```
# MJPEG
gst-launch-1.0 -v v4l2src device=/dev/video0 ! image/jpeg !  jpegdec ! video/x-raw,width=1920,height=1080,framerate=30/1 ! autovideosink
gst-launch-1.0 -v v4l2src device=/dev/video0 ! jpegdec ! video/x-raw,width=1920,height=1080,framerate=30/1 ! autovideosink

# YUYV
gst-launch-1.0 -v  v4l2src device="/dev/video0" ! video/x-raw,width=1920,height=1080,framerate=5/1 ! videoconvert ! ximagesink

# TO FILE
gst-launch-1.0 -v v4l2src device=/dev/video0 ! jpegdec ! video/x-raw,width=1920,height=1080,framerate=30/1 ! x264enc qp-min=18 ! avimux ! filesink location=videotestsrc.avi

# FROM UDP USERMODE
gst-launch-1.0 -v v4l2src device=/dev/video0 ! jpegdec ! video/x-raw,width=1920,height=1080,framerate=30/1 ! x264enc tune=zerolatency bitrate=1024 ! rtph264pay ! udpsink host=127.0.0.1 port=1234

# TO UDP USERMODE
gst-launch-1.0 udpsrc port=1245 caps = "application/x-rtp, media=video, clock-rate=90000, encoding-name=H264, payload=96" ! rtph264depay ! avdec_h264 ! autovideosink

# FROM UDP DIRECT
gst-launch-1.0 -v v4l2src device=/dev/video0 ! jpegdec ! video/x-raw,width=1920,height=1080,framerate=30/1 ! x264enc tune=zerolatency bitrate=1024 ! rtph264pay ! udpsink host=127.0.0.1 port=9923

# TO UDP DIRECT
gst-launch-1.0 udpsrc port=9923 caps = "application/x-rtp, media=video, clock-rate=90000, encoding-name=H264, payload=96" ! rtph264depay ! avdec_h264 ! autovideosink


```