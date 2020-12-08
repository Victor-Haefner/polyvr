/* gdkglext-config.h
 *
 * This is a generated file.  Please modify `configure.in'
 */

#ifndef GDKGLEXT_CONFIG_H
#define GDKGLEXT_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef _WINDOWS
#define GDKGLEXT_WINDOWING_WIN32
#else
#define GDKGLEXT_WINDOWING_X11
#endif

#define GDKGLEXT_MULTIHEAD_SUPPORT

#if !defined(GDKGLEXT_MULTIHEAD_SUPPORT) && defined(GDK_MULTIHEAD_SAFE)
#error "Installed GdkGLExt library doesn't have multihead support."
#endif



#define GDKGLEXT_NEED_GLXFBCONFIGIDSGIX_TYPEDEF





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GDKGLEXT_CONFIG_H */
