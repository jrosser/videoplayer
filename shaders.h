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

	    "uniform int  field;\n"

	    "uniform mat4 colour_matrix;\n"

	    "void main(void) {\n"
	    " float nx,ny;\n"
	    " vec4 yuv;\n"

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

	    " yuv[3]=1.0;\n"

	    " gl_FragColor = colour_matrix * yuv;\n"
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
 */
static const char
    *shaderUYVYDeintSrc=
	    /* "#extension GL_ARB_texture_rectangle : require\n" */
	    "uniform sampler2DRect Ytex;\n"
	    "uniform float CHsubsample, CVsubsample;\n"

	    "uniform int  field;\n"

	    "uniform mat4 colour_matrix;\n"

	    "void main(void) {\n"
	    " float nx,ny;\n"
	    " vec4 yuv;\n"
	    " vec4 rgba_yy, rgba_uv;\n"
	    " vec4 above_yy, above_uv;\n"
	    " vec4 below_yy, below_uv;\n"

	    " nx = gl_TexCoord[0].x;\n"
	    " ny = gl_TexCoord[0].y;\n"

	    " if(mod(floor(ny) + float(field), 2.0) > 0.5) {\n"
	    "     //interpolated line in a field\n"
	    "     above_yy = texture2DRect(Ytex, vec2(floor(nx)+0.5, ny+1.0));\n"
	    "     above_uv = texture2DRect(Ytex, vec2(nx+0.25, ny+1.0));\n"
	    "     below_yy = texture2DRect(Ytex, vec2(floor(nx)+0.5, ny-1.0));\n"
	    "     below_uv = texture2DRect(Ytex, vec2(nx+0.25, ny-1.0));\n"

	    "     yuv[0]  = (fract(nx) < 0.5) ? above_yy.g : above_yy.a;\n"
	    "     yuv[0] += (fract(nx) < 0.5) ? below_yy.g : below_yy.a;\n"
	    "     yuv[1] = above_uv.r + below_uv.r;\n"
	    "     yuv[2] = above_uv.b + below_uv.b;\n"
	    "     yuv = yuv / 2.0;"
	    " }\n"
	    " else {\n"
	    "     //non interpolated line in a field\n"
	    "     rgba_yy = texture2DRect(Ytex, vec2(floor(nx)+0.5, ny));\n"
	    "     rgba_uv = texture2DRect(Ytex, vec2(nx+0.25, ny));\n"
	    "     yuv[0] = (fract(nx) < 0.5) ? rgba_yy.g : rgba_yy.a;\n" //pick the correct luminance from G or A for this pixel
	    "     yuv[1] = rgba_uv.r;\n"
	    "     yuv[2] = rgba_uv.b;\n"
	    " }\n"

	    " yuv[3] = 1.0;\n"

	    " gl_FragColor = colour_matrix * yuv;\n"
	    "}\n";
#endif
