#ifndef __SHADERS_H
#define __SHADERS_H

/* NB, http://opengl.org/registry/specs/ARB/texture_rectangle.txt states:
 * If the string "GL_ARB_texture_rectangle" is present in the
 * EXTENSIONS string, as queried with GetString(), then the compiler will
 * behave as if
 *     #extension GL_ARB_texture_rectangle : require
 * is present in the shader.
 */

/*---------------------------------------------------------------------------*/
/* shader program for planar (progressive) video formats
 * should work with all planar chroma subsamplings
 * glInternalFormat=GL_LUMINANCE glFormat=GL_LUMINANCE glType=GL_UNSIGNED_BYTE
 */
static const char
    *shaderPlanarProSrc=
	    /* "#extension GL_ARB_texture_rectangle : require\n" */
	    "uniform sampler2DRect Ytex;\n"
	    "uniform sampler2DRect Utex,Vtex;\n"
	    "uniform float CHsubsample, CVsubsample;\n"

	    "uniform mat4 colour_matrix;\n"

	    "void main(void) {\n"
	    " float nx,ny;\n"
	    " vec4 yuv;\n"

	    " nx=gl_TexCoord[0].x;\n"
	    " ny=gl_TexCoord[0].y;\n"

	    " yuv[0]=texture2DRect(Ytex,vec2(nx,ny)).r;\n"
	    " yuv[1]=texture2DRect(Utex,vec2(nx/CHsubsample, ny/CVsubsample)).r;\n"
	    " yuv[2]=texture2DRect(Vtex,vec2(nx/CHsubsample, ny/CVsubsample)).r;\n"
	    " yuv[3]=1.0;\n"

	    " gl_FragColor = colour_matrix * yuv;\n"
	    "}\n";

/*---------------------------------------------------------------------------*/
/* shader program for planar (de-interlaced) video formats
 * should work with all planar chroma subsamplings
 * glInternalFormat=GL_LUMINANCE glFormat=GL_LUMINANCE glType=GL_UNSIGNED_BYTE
 */
static const char
    *shaderPlanarDeintSrc=
	    /* "#extension GL_ARB_texture_rectangle : require\n" */
	    "uniform sampler2DRect Ytex;\n"
	    "uniform sampler2DRect Utex,Vtex;\n"
	    "uniform float CHsubsample, CVsubsample;\n"

	    "uniform float  field;\n"

	    "uniform mat4 colour_matrix;\n"

	    "void main(void) {\n"
	    " float luma_x, luma_y, luma_y_orig;\n"
	    " float chroma_x, chroma_y, chroma_y_orig;\n"
	    " vec3 yuv;\n"

	    " luma_x = gl_TexCoord[0].x;\n"
	    " luma_y_orig = gl_TexCoord[0].y;\n"
	    /* luma_y is the nearest Y texel-centre above or co-sited with luma_y_orig
	     * taking into account that in interlaced (ppf) video, there is a vertical
	     * seperation of 2 between adjacent lines belonging to the same field */
	    " luma_y = field + 0.5 + 2.0 * floor((luma_y_orig -field -0.5) / 2.0);\n"

	    " chroma_x = gl_TexCoord[0].x / CHsubsample;\n"
	    " chroma_y_orig = gl_TexCoord[0].y / CVsubsample;\n"
	    /* chroma_y is the nearest Cb/Cr texel-centre above or co-sited with
	     * chroma_y_orig, taking into account interlace (as above) */
	    " chroma_y = field + 0.5 + 2.0 * floor((chroma_y_orig -field -0.5) / 2.0);\n"

	    /* yuv_above are the nearest texel-centred YCbCr samples above the
	     *           current output raster pos */
	    /* yuv_below are the nearest texel-centred YCbCr samples below the
	     *           current output raster pos */
	    " vec3 yuv_above, yuv_below;\n"
	    " yuv_above[0] = texture2DRect(Ytex, vec2(luma_x, luma_y)).r;\n"
	    " yuv_below[0] = texture2DRect(Ytex, vec2(luma_x, luma_y + 2.0)).r;\n"

	    " yuv_above[1] = texture2DRect(Utex, vec2(chroma_x, chroma_y)).r;\n"
	    " yuv_above[2] = texture2DRect(Vtex, vec2(chroma_x, chroma_y)).r;\n"
	    " yuv_below[1] = texture2DRect(Utex, vec2(chroma_x, chroma_y + 2.0)).r;\n"
	    " yuv_below[2] = texture2DRect(Vtex, vec2(chroma_x, chroma_y + 2.0)).r;\n"

	    /* These weights are the normalized distance from the orig pixel
	     * to the top centre-exact texture coordinate */
	    " float luma_weight = (luma_y_orig - luma_y) / 2.0;\n"
	    " float chroma_weight = (chroma_y_orig - chroma_y) / 2.0;\n"
	    " vec3 a = vec3(luma_weight, chroma_weight, chroma_weight);\n"
	    " yuv = mix(yuv_above, yuv_below, a);\n"

	    " gl_FragColor = colour_matrix * vec4(yuv, 1.0);\n"
	    "}\n";

/*---------------------------------------------------------------------------*/
/* shader program for muxed (progressive) video:
 * - 8 bit = glInternalFormat=GL_RGBA glFormat=GL_RGBA glType=GL_UNSIGNED_BYTE
 * -16 bit = glInternalFormat=GL_RGBA glFormat=GL_RGBA glType=GL_UNSIGNED_SHORT
 *
 * there are 2 luminance samples per RGBA quad, so divide the horizontal
 * location by two to use each RGBA value twice.
 */
static const char
    *shaderUYVYProSrc=
	    /* "#extension GL_ARB_texture_rectangle : require\n" */
	    "uniform sampler2DRect Ytex;\n"
	    "uniform float CHsubsample, CVsubsample;\n"

	    "uniform mat4 colour_matrix;\n"

	    "void main(void) {\n"
	    " float nx,ny;\n"
	    " vec4 yuv;\n"
	    " vec4 yy, uv;\n"

	    " nx = gl_TexCoord[0].x;\n"
	    " ny = gl_TexCoord[0].y;\n"

	    /* 0.25 is added to move the chroma_x position onto:
	     *  - texel centres, when luma & chroma are co-sited
	     *  - between texels, for luma samples without co-sited chroma */
	    " uv = texture2DRect(Ytex, vec2(nx+0.25, ny));\n" //sample with linear interpolation for chrominance
	    " yy = texture2DRect(Ytex, vec2(floor(nx)+0.5, ny));\n" //sample each RGBA quad with no linear interpolation for luminance

	    " yuv[0] = (fract(nx) < 0.5) ? yy.g : yy.a;\n" //pick the correct luminance from G or A for this pixel
	    " yuv[1] = uv.r;\n"
	    " yuv[2] = uv.b;\n"
	    " yuv[3] = 1.0;\n"

	    " gl_FragColor = colour_matrix * yuv;\n"
	    "}\n";

/*---------------------------------------------------------------------------*/
/* shader program for muxed (interlaced) video:
 * - 8 bit = glInternalFormat=GL_RGBA glFormat=GL_RGBA glType=GL_UNSIGNED_BYTE
 * -16 bit = glInternalFormat=GL_RGBA glFormat=GL_RGBA glType=GL_UNSIGNED_SHORT
 *
 * there are 2 luminance samples per RGBA quad, so divide the horizontal
 * location by two to use each RGBA value twice.
 *
 * This shader only works correctly when there is:
 *   - a 1:1 horizontal pixel mapping
 */
static const char
    *shaderUYVYDeintSrc=
	    /* "#extension GL_ARB_texture_rectangle : require\n" */
	    "uniform sampler2DRect Ytex;\n"
	    "uniform float CHsubsample, CVsubsample;\n"

	    "uniform float field;\n"

	    "uniform mat4 colour_matrix;\n"

	    "void main(void) {\n"
	    " float luma_x, luma_y, luma_y_orig;\n"
	    " float chroma_x, chroma_y, chroma_y_orig;\n"
	    " vec4 yuv;\n"

	    " luma_x = gl_TexCoord[0].x;\n"
	    " luma_y_orig = gl_TexCoord[0].y;\n"
	    /* luma_y is the nearest Y texel-centre above or co-sited with luma_y_orig
	     * taking into account that in interlaced (ppf) video, there is a vertical
	     * seperation of 2 between adjacent lines belonging to the same field */
	    " luma_y = field + 0.5 + 2.0 * floor((gl_TexCoord[0].y -field -0.5) / 2.0);\n"

	    /* chroma_x is coordinates don't need dividing by two due to the way
	     * they are muxed implies a multiplication by two */
	    " chroma_x = gl_TexCoord[0].x;\n"
	    " chroma_y_orig = gl_TexCoord[0].y / CVsubsample;\n"
	    /* chroma_y is the nearest Cb/Cr texel-centre above or co-sited with
	     * chroma_y_orig, taking into account interlace (as above) */
	    " chroma_y = field + 0.5 + 2.0 * floor((gl_TexCoord[0].y/CVsubsample -field -0.5) / 2.0);\n"

	    " vec4 yuv_above, yuv_below;\n"
	    /* To make use of SIMD, we act on a fourword quad, and only select which
	     * horizontal luma sample to use after vertical averaging, since we
	     * have an empty word sitting aronud */
	    " yuv_above.ra = texture2DRect(Ytex, vec2(floor(luma_x)+0.5, luma_y)).ga;\n"
	    " yuv_below.ra = texture2DRect(Ytex, vec2(floor(luma_x)+0.5, luma_y + 2.0)).ga;\n"

	    /* 0.25 is added to move the chroma_x position onto:
	     *  - texel centres, when luma & chroma are co-sited
	     *  - between texels, for luma samples without co-sited chroma */
	    " yuv_above.gb = texture2DRect(Ytex, vec2(chroma_x+0.25, chroma_y)).rb;\n"
	    " yuv_below.gb = texture2DRect(Ytex, vec2(chroma_x+0.25, chroma_y + 2.0)).rb;\n"

	    /* These weights are the normalized distance from the orig pixel
	     * to the top centre-exact texture coordinate */
	    " float luma_weight = (luma_y_orig - luma_y) / 2.0;\n"
	    " float chroma_weight = (chroma_y_orig - chroma_y) / 2.0;\n"
	    " vec4 a = vec4(luma_weight, chroma_weight, chroma_weight, luma_weight);\n"
	    " yuv = mix(yuv_above, yuv_below, a);\n"

	    /* pick which luma sample to use */
	    " yuv.r = (fract(luma_x) < 0.5) ? yuv.r : yuv.a;\n"
	    " yuv.a = 1.0;\n"

	    " gl_FragColor = colour_matrix * yuv;\n"
	    "}\n";
#endif
