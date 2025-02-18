/*
 * Copyright (C) 2017, 2020 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ColorUtilities.h"

#include "ColorMatrix.h"
#include <wtf/MathExtras.h>

namespace WebCore {

ColorComponents::ColorComponents(const FloatComponents& floatComponents)
{
    components[0] = clampedColorComponent(floatComponents.components[0]);
    components[1] = clampedColorComponent(floatComponents.components[1]);
    components[2] = clampedColorComponent(floatComponents.components[2]);
    components[3] = clampedColorComponent(floatComponents.components[3]);
}

bool areEssentiallyEqual(const FloatComponents& a, const FloatComponents& b)
{
    return WTF::areEssentiallyEqual(a.components[0], b.components[0])
        && WTF::areEssentiallyEqual(a.components[1], b.components[1])
        && WTF::areEssentiallyEqual(a.components[2], b.components[2])
        && WTF::areEssentiallyEqual(a.components[3], b.components[3]);
}

// These are the standard sRGB <-> linearRGB conversion functions (https://en.wikipedia.org/wiki/SRGB).
float linearToRGBColorComponent(float c)
{
    if (c < 0.0031308f)
        return 12.92f * c;

    return clampTo<float>(1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f, 0, 1);
}

float rgbToLinearColorComponent(float c)
{
    if (c <= 0.04045f)
        return c / 12.92f;

    return clampTo<float>(std::pow((c + 0.055f) / 1.055f, 2.4f), 0, 1);
}

FloatComponents rgbToLinearComponents(const FloatComponents& RGBColor)
{
    return {
        rgbToLinearColorComponent(RGBColor.components[0]),
        rgbToLinearColorComponent(RGBColor.components[1]),
        rgbToLinearColorComponent(RGBColor.components[2]),
        RGBColor.components[3]
    };
}

FloatComponents linearToRGBComponents(const FloatComponents& linearRGB)
{
    return {
        linearToRGBColorComponent(linearRGB.components[0]),
        linearToRGBColorComponent(linearRGB.components[1]),
        linearToRGBColorComponent(linearRGB.components[2]),
        linearRGB.components[3]
    };
}

static FloatComponents xyzToLinearSRGB(const FloatComponents& XYZComponents)
{
    // https://en.wikipedia.org/wiki/SRGB
    // http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
    const float values[] = {
         3.2404542f, -1.5371385f, -0.4985314f, 0.0f, 0.0f,
        -0.9692660f,  1.8760108f,  0.0415560f, 0.0f, 0.0f,
         0.0556434f, -0.2040259f,  1.0572252f, 0.0f, 0.0f,
         0.0f,        0.0f,        0.0f,       1.0f, 0.0f,
    };
    ColorMatrix xyzToLinearSRGBMatrix(values);
    return xyzToLinearSRGBMatrix.transformedColorComponents(XYZComponents);
}

static FloatComponents linearSRGBToXYZ(const FloatComponents& XYZComponents)
{
    // https://en.wikipedia.org/wiki/SRGB
    // http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
    const float values[] = {
        0.4124564f,  0.3575761f,  0.1804375f, 0.0f, 0.0f,
        0.2126729f,  0.7151522f,  0.0721750f, 0.0f, 0.0f,
        0.0193339f,  0.1191920f,  0.9503041f, 0.0f, 0.0f,
        0.0f,        0.0f,        0.0f,       1.0f, 0.0f,
    };
    ColorMatrix linearSRGBToXYZMatrix(values);
    return linearSRGBToXYZMatrix.transformedColorComponents(XYZComponents);
}

static FloatComponents XYZToLinearP3(const FloatComponents& XYZComponents)
{
    // https://drafts.csswg.org/css-color/#color-conversion-code
    const float values[] = {
         2.493496911941425f,  -0.9313836179191239f, -0.4027107844507168f, 0.0f, 0.0f,
        -0.8294889695615747f,  1.7626640603183463f,  0.0236246858419436f, 0.0f, 0.0f,
         0.0358458302437845f, -0.0761723892680418f,  0.9568845240076872f, 0.0f, 0.0f,
         0.0f,                 0.0f,                 0.0f,                1.0f, 0.0f,
    };
    ColorMatrix xyzToLinearSRGBMatrix(values);
    return xyzToLinearSRGBMatrix.transformedColorComponents(XYZComponents);
}

static FloatComponents linearP3ToXYZ(const FloatComponents& XYZComponents)
{
    // https://drafts.csswg.org/css-color/#color-conversion-code
    const float values[] = {
        0.4865709486482162f, 0.2656676931690931f, 0.198217285234363f, 0.0f, 0.0f,
        0.2289745640697488f, 0.6917385218365064f, 0.079286914093745f, 0.0f, 0.0f,
        0.0f,                0.0451133818589026f, 1.043944368900976f, 0.0f, 0.0f,
        0.0f,                0.0f,                0.0f,               1.0f, 0.0f,
    };
    ColorMatrix linearP3ToXYZMatrix(values);
    return linearP3ToXYZMatrix.transformedColorComponents(XYZComponents);
}

FloatComponents p3ToSRGB(const FloatComponents& p3)
{
    auto linearP3 = rgbToLinearComponents(p3);
    auto xyz = linearP3ToXYZ(linearP3);
    auto linearSRGB = xyzToLinearSRGB(xyz);
    return linearToRGBComponents(linearSRGB);
}

FloatComponents sRGBToP3(const FloatComponents& sRGB)
{
    auto linearSRGB = rgbToLinearComponents(sRGB);
    auto xyz = linearSRGBToXYZ(linearSRGB);
    auto linearP3 = XYZToLinearP3(xyz);
    return linearToRGBComponents(linearP3);
}

float lightness(const FloatComponents& sRGBCompontents)
{
    auto [r, g, b, a] = sRGBCompontents;

    float max = std::max({ r, g, b });
    float min = std::min({ r, g, b });

    return 0.5f * (max + min);
}

// This is similar to sRGBToLinearColorComponent but for some reason
// https://www.w3.org/TR/2008/REC-WCAG20-20081211/#relativeluminancedef
// doesn't use the standard sRGB -> linearRGB threshold of 0.04045.
static float sRGBToLinearColorComponentForLuminance(float c)
{
    if (c <= 0.03928f)
        return c / 12.92f;

    return clampTo<float>(std::pow((c + 0.055f) / 1.055f, 2.4f), 0, 1);
}

float luminance(const FloatComponents& sRGBComponents)
{
    // Values from https://www.w3.org/TR/2008/REC-WCAG20-20081211/#relativeluminancedef
    return 0.2126f * sRGBToLinearColorComponentForLuminance(sRGBComponents.components[0])
        + 0.7152f * sRGBToLinearColorComponentForLuminance(sRGBComponents.components[1])
        + 0.0722f * sRGBToLinearColorComponentForLuminance(sRGBComponents.components[2]);
}

float contrastRatio(const FloatComponents& componentsA, const FloatComponents& componentsB)
{
    // Uses the WCAG 2.0 definition of contrast ratio.
    // https://www.w3.org/TR/WCAG20/#contrast-ratiodef
    float lighterLuminance = luminance(componentsA);
    float darkerLuminance = luminance(componentsB);

    if (lighterLuminance < darkerLuminance)
        std::swap(lighterLuminance, darkerLuminance);

    return (lighterLuminance + 0.05) / (darkerLuminance + 0.05);
}

FloatComponents sRGBToHSL(const FloatComponents& sRGBCompontents)
{
    // http://en.wikipedia.org/wiki/HSL_color_space.
    auto [r, g, b, alpha] = sRGBCompontents;

    float max = std::max({ r, g, b });
    float min = std::min({ r, g, b });
    float chroma = max - min;

    float hue;
    if (!chroma)
        hue = 0;
    else if (max == r)
        hue = (60.0f * ((g - b) / chroma)) + 360.0f;
    else if (max == g)
        hue = (60.0f * ((b - r) / chroma)) + 120.0f;
    else
        hue = (60.0f * ((r - g) / chroma)) + 240.0f;

    if (hue >= 360.0f)
        hue -= 360.0f;

    hue /= 360.0f;

    float lightness = 0.5f * (max + min);
    float saturation;
    if (!chroma)
        saturation = 0;
    else if (lightness <= 0.5f)
        saturation = (chroma / (max + min));
    else
        saturation = (chroma / (2.0f - (max + min)));

    return {
        hue,
        saturation,
        lightness,
        alpha
    };
}

// Hue is in the range 0-6, other args in 0-1.
static float calcHue(float temp1, float temp2, float hueVal)
{
    if (hueVal < 0.0f)
        hueVal += 6.0f;
    else if (hueVal >= 6.0f)
        hueVal -= 6.0f;
    if (hueVal < 1.0f)
        return temp1 + (temp2 - temp1) * hueVal;
    if (hueVal < 3.0f)
        return temp2;
    if (hueVal < 4.0f)
        return temp1 + (temp2 - temp1) * (4.0f - hueVal);
    return temp1;
}

// Explanation of this algorithm can be found in the CSS Color 4 Module
// specification at https://drafts.csswg.org/css-color-4/#hsl-to-rgb with
// further explanation available at http://en.wikipedia.org/wiki/HSL_color_space
FloatComponents hslToSRGB(const FloatComponents& hslColor)
{
    float hue = hslColor.components[0];
    float saturation = hslColor.components[1];
    float lightness = hslColor.components[2];

    // Convert back to RGB.
    if (!saturation) {
        return {
            lightness,
            lightness,
            lightness,
            hslColor.components[3]
        };
    }
    
    float temp2 = lightness <= 0.5f ? lightness * (1.0f + saturation) : lightness + saturation - lightness * saturation;
    float temp1 = 2.0f * lightness - temp2;
    
    hue *= 6.0f; // calcHue() wants hue in the 0-6 range.
    return {
        calcHue(temp1, temp2, hue + 2.0f),
        calcHue(temp1, temp2, hue),
        calcHue(temp1, temp2, hue - 2.0f),
        hslColor.components[3]
    };
}

FloatComponents premultiplied(const FloatComponents& sRGBComponents)
{
    auto [r, g, b, a] = sRGBComponents;
    return {
        r * a,
        g * a,
        b * a,
        a
    };
}

} // namespace WebCore
