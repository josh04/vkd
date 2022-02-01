
float _linearToSRGB(const float j) {
    float i = clamp(j, 0.0, 1.0);
    const float a = 0.055f;
    if (i > 0.0031308) {
        return (1 + a) * pow(i, 1.0f / 2.4f) - a;
    } else {
        return 12.92 * i;
    }
    return pow(i, 0.4545f);
}

float _SRGBtoLinear(const float i) {
    const float a = 0.055f;
    if (i > 0.04045) {
        return pow((i + a)/(1+a), 2.4f);
    } else {
        return i/12.92;
    }
}

vec4 linearToSRGB(const vec4 i) {
    return vec4(_linearToSRGB(i.x), _linearToSRGB(i.y), _linearToSRGB(i.z), i.w);
}


vec4 SRGBtoLinear(const vec4 i) {
    return vec4(_SRGBtoLinear(i.x), _SRGBtoLinear(i.y), _SRGBtoLinear(i.z), i.w);
}