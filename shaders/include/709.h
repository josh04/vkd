
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

float rec709backward(float v) {
    const float a = 0.099;
    
    float x;
    if (v < 0.081) {
        x = v / 4.5;
    } else {
        x = pow((v + a) / (1.0 + a), 1 / 0.45);
    }
    return x;
}

float rec709forward(float l) {
    float v;
    
    if (l < 0.018) {
        v = 4.5 * l;
    } else {
        v = (1.099 * pow(l, 0.45f)) - 0.099;
    }
    
    return v;
}

vec4 lineartorec709(vec4 in_a) {
	return vec4(rec709forward(in_a.x), rec709forward(in_a.y), rec709forward(in_a.z), 1.0f);
}

vec4 rec709tolinear(vec4 in_a) {
	return vec4(rec709backward(in_a.x), rec709backward(in_a.y), rec709backward(in_a.z), 1.0f);
}
