
vec4 bt709rgbyuv(const vec4 rgb) {
    const float y = rgb.x*0.2126 +    rgb.y*0.7152 +    rgb.z*0.0722;
    const float u = rgb.x*-0.1146 +   rgb.y*-0.3854 +   rgb.z*0.5;
    const float v = rgb.x*0.5 +       rgb.y*-0.4542 +   rgb.z*-0.0458;
     
    return vec4(y, (u*1.023f)+0.5f, (v*1.023f)+0.5f, rgb.w);
}

vec4 bt709yuvrgb(vec4 yuv) {
    yuv.y = yuv.y - 0.5;
    yuv.z = yuv.z - 0.5;
    const float r = yuv.x*1 + yuv.y*0 +         yuv.z*1.570;
    const float g = yuv.x*1 + yuv.y*-0.187 +    yuv.z*-0.467;
    const float b = yuv.x*1 + yuv.y*1.856 +     yuv.z*0;
    return vec4(r, g, b, yuv.w);
}
