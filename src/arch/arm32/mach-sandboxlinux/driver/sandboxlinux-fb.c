/*
 * driver/sandboxlinux-fb.c
 *
 * Copyright(c) 2007-2014 Jianjun Jiang <8192542@qq.com>
 * Official site: http://xboot.org
 * Mobile phone: +86-18665388956
 * QQ: 8192542
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <xboot.h>
#include <fb/fb.h>
#include <sandboxlinux.h>

static void fb_init(struct fb_t * fb)
{
	struct sandbox_config_t * cfg = sandbox_linux_get_config();
	sandbox_linux_sdl_fb_init(cfg->framebuffer.width, cfg->framebuffer.height);
}

static void fb_exit(struct fb_t * fb)
{
	sandbox_linux_sdl_fb_exit();
}

static int fb_ioctl(struct fb_t * fb, int cmd, void * arg)
{
	struct sandbox_config_t * cfg = sandbox_linux_get_config();
	struct screen_info_t * info;
	int * brightness;

	switch(cmd)
	{
	case IOCTL_FB_GET_SCREEN_INFORMATION:
		info = (struct screen_info_t *)arg;
		info->width = cfg->framebuffer.width;
		info->height = cfg->framebuffer.height;
		info->xdpi = cfg->framebuffer.xdpi;
		info->ydpi = cfg->framebuffer.ydpi;
		info->bpp = 32;
		return 0;

	case IOCTL_FB_SET_BACKLIGHT_BRIGHTNESS:
		brightness = (int *)arg;
		sandbox_linux_sdl_fb_set_backlight(*brightness);
		return 0;

	case IOCTL_FB_GET_BACKLIGHT_BRIGHTNESS:
		brightness = (int *)arg;
		*brightness = sandbox_linux_sdl_fb_get_backlight();
		return 0;

	default:
		break;
	}

	return -1;
}

struct render_t * fb_create(struct fb_t * fb)
{
	struct sandbox_config_t * cfg = sandbox_linux_get_config();
	struct sandbox_fb_surface_t * surface;
	struct render_t * render;

	surface = malloc(sizeof(struct sandbox_fb_surface_t));
	if(!surface)
		return NULL;

	if(sandbox_linux_sdl_fb_surface_create(surface, cfg->framebuffer.width, cfg->framebuffer.height) != 0)
	{
		free(surface);
		return NULL;
	}

	render = malloc(sizeof(struct render_t));
	if(!render)
	{
		sandbox_linux_sdl_fb_surface_destroy(surface);
		free(surface);
		return NULL;
	}

	render->width = surface->width;
	render->height = surface->height;
	render->pitch = surface->pitch;
	render->format = PIXEL_FORMAT_ARGB32;
	render->pixels = surface->pixels;
	render->pixlen = surface->height * surface->pitch;
	render->priv = surface;

	render->clear = sw_render_clear;
	render->snapshot = sw_render_snapshot;
	render->alloc_texture = sw_render_alloc_texture;
	render->alloc_texture_similar = sw_render_alloc_texture_similar;
	render->free_texture = sw_render_free_texture;
	render->fill_texture = sw_render_fill_texture;
	render->blit_texture = sw_render_blit_texture;
	sw_render_create_data(render);

	return render;
}

void fb_destroy(struct fb_t * fb, struct render_t * render)
{
	if(render)
	{
		sandbox_linux_sdl_fb_surface_destroy(render->priv);
		free(render->priv);
		free(render);
	}
}

void fb_present(struct fb_t * fb, struct render_t * render)
{
	sandbox_linux_sdl_fb_surface_present(render->priv);
}

static void fb_suspend(struct fb_t * fb)
{
}

static void fb_resume(struct fb_t * fb)
{
}

static bool_t sandboxlinux_register_framebuffer(struct resource_t * res)
{
	struct fb_t * fb;
	char name[64];

	fb = malloc(sizeof(struct fb_t));
	if(!fb)
		return FALSE;

	snprintf(name, sizeof(name), "%s.%d", res->name, res->id);

	fb->name = strdup(name);
	fb->init = fb_init,
	fb->exit = fb_exit,
	fb->ioctl = fb_ioctl,
	fb->create = fb_create,
	fb->destroy = fb_destroy,
	fb->present = fb_present,
	fb->suspend = fb_suspend,
	fb->resume = fb_resume,
	fb->priv = res;

	if(register_framebuffer(fb))
		return TRUE;

	free(fb->name);
	free(fb);
	return FALSE;
}

static bool_t sandboxlinux_unregister_framebuffer(struct resource_t * res)
{
	struct fb_t * fb;
	char name[64];

	snprintf(name, sizeof(name), "%s.%d", res->name, res->id);

	fb = search_framebuffer(name);
	if(!fb)
		return FALSE;

	if(!unregister_framebuffer(fb))
		return FALSE;

	free(fb->name);
	free(fb);
	return TRUE;
}

static __init void sandboxlinux_fb_init(void)
{
	resource_for_each_with_name("sandboxlinux-fb", sandboxlinux_register_framebuffer);
}

static __exit void sandboxlinux_fb_exit(void)
{
	resource_for_each_with_name("sandboxlinux-fb", sandboxlinux_unregister_framebuffer);
}

device_initcall(sandboxlinux_fb_init);
device_exitcall(sandboxlinux_fb_exit);