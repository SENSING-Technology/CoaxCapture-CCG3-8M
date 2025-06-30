#!/bin/bash
rm *.jpeg
ffmpeg -s 1280x720  -pix_fmt uyvy422 -i frame_0_60.yuv test_0.jpeg
ffmpeg -s 1280x720  -pix_fmt uyvy422 -i frame_1_60.yuv test_1.jpeg
ffmpeg -s 1280x720  -pix_fmt uyvy422 -i frame_2_60.yuv test_2.jpeg
ffmpeg -s 1280x720  -pix_fmt uyvy422 -i frame_3_60.yuv test_3.jpeg
ffmpeg -s 1280x720  -pix_fmt uyvy422 -i frame_4_60.yuv test_4.jpeg
ffmpeg -s 1280x720  -pix_fmt uyvy422 -i frame_5_60.yuv test_5.jpeg
ffmpeg -s 1280x720  -pix_fmt uyvy422 -i frame_6_60.yuv test_6.jpeg
ffmpeg -s 1280x720  -pix_fmt uyvy422 -i frame_7_60.yuv test_7.jpeg
