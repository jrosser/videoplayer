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

	    "uniform vec3 yuvOffset1;\n"
	    "uniform vec3 yuvMul;\n"
	    "uniform vec3 yuvOffset2;\n"
	    "uniform mat3 colourMatrix;\n"

	    "void main(void) {\n"
	    " float nx,ny,a;\n"
	    " vec3 yuv;\n"
	    " vec3 rgb;\n"

	    " nx=gl_TexCoord[0].x;\n"
	    " ny=gl_TexCoord[0].y;\n"

	    " yuv[0]=texture2DRect(Ytex,vec2(nx,ny)).r;\n"
	    " yuv[1]=texture2DRect(Utex,vec2(nx/CHsubsample, ny/CVsubsample)).r;\n"
	    " yuv[2]=texture2DRect(Vtex,vec2(nx/CHsubsample, ny/CVsubsample)).r;\n"

	    " yuv = yuv + yuvOffset1;\n"
	    " yuv = yuv * yuvMul;\n"
	    " yuv = yuv + yuvOffset2;\n"
	    " rgb = yuv * colourMatrix;\n"

	    "    a=1.0;\n"

	    " gl_FragColor=vec4(rgb ,a);\n"
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

	    "uniform int  field;\n"
	    "uniform vec3 yuvOffset1;\n"
	    "uniform vec3 yuvMul;\n"
	    "uniform vec3 yuvOffset2;\n"
	    "uniform mat3 colourMatrix;\n"

	    "void main(void) {\n"
	    " float nx,ny,a;\n"
	    " vec3 yuv;\n"
	    " vec3 rgb;\n"

	    " nx=gl_TexCoord[0].x;\n"
	    " ny=gl_TexCoord[0].y;\n"

	    " if(mod(floor(ny) + float(field), 2.0) > 0.5) {\n"
	    "     //interpolated line in a field\n"
	    "     yuv[0] = texture2DRect(Ytex,vec2(nx, (ny+1.0))).r + texture2DRect(Ytex,vec2(nx, (ny-1.0))).r;\n"
	    "     yuv[1] = texture2DRect(Utex,vec2(nx/CHsubsample, (ny+1.0)/CVsubsample)).r + texture2DRect(Utex,vec2(nx/CHsubsample, (ny-1.0)/CVsubsample)).r;\n"
	    "     yuv[2] = texture2DRect(Vtex,vec2(nx/CHsubsample, (ny+1.0)/CVsubsample)).r + texture2DRect(Vtex,vec2(nx/CHsubsample, (ny-1.0)/CVsubsample)).r;\n"
	    "     yuv = yuv / 2.0;"
	    " }\n"
	    " else {\n"
	    "     //non interpolated line in a field\n"
	    "     yuv[0]=texture2DRect(Ytex,vec2(nx,ny)).r;\n"
	    "     yuv[1]=texture2DRect(Utex,vec2(nx/CHsubsample, ny/CVsubsample)).r;\n"
	    "     yuv[2]=texture2DRect(Vtex,vec2(nx/CHsubsample, ny/CVsubsample)).r;\n"
	    " }\n"

	    " yuv = yuv + yuvOffset1;\n"
	    " yuv = yuv * yuvMul;\n"
	    " yuv = yuv + yuvOffset2;\n"
	    " rgb = yuv * colourMatrix;\n"

	    "    a=1.0;\n"

	    " gl_FragColor=vec4(rgb ,a);\n"
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

	    "uniform vec3 yuvOffset1;\n"
	    "uniform vec3 yuvMul;\n"
	    "uniform vec3 yuvOffset2;\n"
	    "uniform mat3 colourMatrix;\n"

	    "void main(void) {\n"
	    " float nx,ny,a;\n"
	    " vec3 yuv;\n"
	    " vec4 rgba;\n"
	    " vec4 above;\n"
	    " vec4 below;\n"
	    " vec3 rgb;\n"

	    " nx=gl_TexCoord[0].x;\n"
	    " ny=gl_TexCoord[0].y;\n"

	    " rgba = texture2DRect(Ytex, vec2(floor(nx), ny));\n" //sample every other RGBA quad to get luminance
	    " yuv[0] = (fract(nx) < 0.5) ? rgba.g : rgba.a;\n" //pick the correct luminance from G or A for this pixel
	    " yuv[1] = rgba.r;\n"
	    " yuv[2] = rgba.b;\n"

	    " yuv = yuv + yuvOffset1;\n"
	    " yuv = yuv * yuvMul;\n"
	    " yuv = yuv + yuvOffset2;\n"
	    " rgb = yuv * colourMatrix;\n"

	    " a=1.0;\n"

	    " gl_FragColor=vec4(rgb, a);\n"
	    "}\n";

/*---------------------------------------------------------------------------*/
/* shader program for muxed (interlaced) video:
 * - 8 bit = glInternalFormat=GL_RGBA glFormat=GL_RGBA glType=GL_UNSIGNED_BYTE
 * -16 bit = glInternalFormat=GL_RGBA glFormat=GL_RGBA glType=GL_UNSIGNED_SHORT
 *
 * there are 2 luminance samples per RGBA quad, so divide the horizontal
 * location by two to use each RGBA value twice.
 */
static const char
    *shaderUYVYDeintSrc=
	    /* "#extension GL_ARB_texture_rectangle : require\n" */
	    "uniform sampler2DRect Ytex;\n"
	    "uniform float CHsubsample, CVsubsample;\n"

	    "uniform int  field;\n"
	    "uniform vec3 yuvOffset1;\n"
	    "uniform vec3 yuvMul;\n"
	    "uniform vec3 yuvOffset2;\n"
	    "uniform mat3 colourMatrix;\n"

	    "void main(void) {\n"
	    " float nx,ny,a;\n"
	    " vec3 yuv;\n"
	    " vec4 rgba;\n"
	    " vec4 above;\n"
	    " vec4 below;\n"
	    " vec3 rgb;\n"

	    " nx=gl_TexCoord[0].x;\n"
	    " ny=gl_TexCoord[0].y;\n"

	    " if(mod(floor(ny) + float(field), 2.0) > 0.5) {\n"
	    "     //interpolated line in a field\n"
	    "     above = texture2DRect(Ytex, vec2(floor(nx), ny+1.0));\n"
	    "     below = texture2DRect(Ytex, vec2(floor(nx), ny-1.0));\n"

	    "     yuv[0]  = (fract(nx) < 0.5) ? above.g : above.a;\n"
	    "     yuv[0] += (fract(nx) < 0.5) ? below.g : below.a;\n"
	    "     yuv[1] = above.r + below.r;\n"
	    "     yuv[2] = above.b + below.b;\n"
	    "     yuv = yuv / 2.0;"
	    " }\n"
	    " else {\n"
	    "     //non interpolated line in a field\n"
	    "     rgba = texture2DRect(Ytex, vec2(floor(nx), ny));\n" //sample every other RGBA quad to get luminance
	    "     yuv[0] = (fract(nx) < 0.5) ? rgba.g : rgba.a;\n" //pick the correct luminance from G or A for this pixel
	    "     yuv[1] = rgba.r;\n"
	    "     yuv[2] = rgba.b;\n"
	    " }\n"

	    " yuv = yuv + yuvOffset1;\n"
	    " yuv = yuv * yuvMul;\n"
	    " yuv = yuv + yuvOffset2;\n"
	    " rgb = yuv * colourMatrix;\n"

	    " a=1.0;\n"

	    " gl_FragColor=vec4(rgb, a);\n"
	    "}\n";
#endif
