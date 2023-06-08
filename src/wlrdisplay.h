#ifndef WLR_OUTPUT_MANAGEMENT_UNSTABLE_V1_CLIENT_PROTOCOL_H
#define WLR_OUTPUT_MANAGEMENT_UNSTABLE_V1_CLIENT_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

#ifdef  __cplusplus
extern "C" {
#endif

struct zwlr_output_configuration_head_v1;
struct zwlr_output_configuration_v1;
struct zwlr_output_head_v1;
struct zwlr_output_manager_v1;
struct zwlr_output_mode_v1;

#ifndef ZWLR_OUTPUT_MANAGER_V1_INTERFACE
#define ZWLR_OUTPUT_MANAGER_V1_INTERFACE

extern const struct wl_interface zwlr_output_manager_v1_interface;
#endif
#ifndef ZWLR_OUTPUT_HEAD_V1_INTERFACE
#define ZWLR_OUTPUT_HEAD_V1_INTERFACE

extern const struct wl_interface zwlr_output_head_v1_interface;
#endif
#ifndef ZWLR_OUTPUT_MODE_V1_INTERFACE
#define ZWLR_OUTPUT_MODE_V1_INTERFACE

extern const struct wl_interface zwlr_output_mode_v1_interface;
#endif
#ifndef ZWLR_OUTPUT_CONFIGURATION_V1_INTERFACE
#define ZWLR_OUTPUT_CONFIGURATION_V1_INTERFACE

extern const struct wl_interface zwlr_output_configuration_v1_interface;
#endif
#ifndef ZWLR_OUTPUT_CONFIGURATION_HEAD_V1_INTERFACE
#define ZWLR_OUTPUT_CONFIGURATION_HEAD_V1_INTERFACE

extern const struct wl_interface zwlr_output_configuration_head_v1_interface;
#endif

struct zwlr_output_manager_v1_listener {
	void (*head)(void *data,
		     struct zwlr_output_manager_v1 *zwlr_output_manager_v1,
		     struct zwlr_output_head_v1 *head);

	void (*done)(void *data,
		     struct zwlr_output_manager_v1 *zwlr_output_manager_v1,
		     uint32_t serial);

	void (*finished)(void *data,
			 struct zwlr_output_manager_v1 *zwlr_output_manager_v1);
};

static inline int
zwlr_output_manager_v1_add_listener(struct zwlr_output_manager_v1 *zwlr_output_manager_v1,
				    const struct zwlr_output_manager_v1_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) zwlr_output_manager_v1,
				     (void (**)(void)) listener, data);
}

static inline void
zwlr_output_manager_v1_destroy(struct zwlr_output_manager_v1 *zwlr_output_manager_v1)
{
	wl_proxy_destroy((struct wl_proxy *) zwlr_output_manager_v1);
}

struct zwlr_output_head_v1_listener {
	void (*name)(void *data,
		     struct zwlr_output_head_v1 *zwlr_output_head_v1,
		     const char *name);

	void (*description)(void *data,
			    struct zwlr_output_head_v1 *zwlr_output_head_v1,
			    const char *description);

	void (*physical_size)(void *data,
			      struct zwlr_output_head_v1 *zwlr_output_head_v1,
			      int32_t width,
			      int32_t height);
	void (*mode)(void *data,
		     struct zwlr_output_head_v1 *zwlr_output_head_v1,
		     struct zwlr_output_mode_v1 *mode);

	void (*enabled)(void *data,
			struct zwlr_output_head_v1 *zwlr_output_head_v1,
			int32_t enabled);

	void (*current_mode)(void *data,
			     struct zwlr_output_head_v1 *zwlr_output_head_v1,
			     struct zwlr_output_mode_v1 *mode);

	void (*position)(void *data,
			 struct zwlr_output_head_v1 *zwlr_output_head_v1,
			 int32_t x,
			 int32_t y);

	void (*transform)(void *data,
			  struct zwlr_output_head_v1 *zwlr_output_head_v1,
			  int32_t transform);

	void (*scale)(void *data,
		      struct zwlr_output_head_v1 *zwlr_output_head_v1,
		      wl_fixed_t scale);

	void (*finished)(void *data,
			 struct zwlr_output_head_v1 *zwlr_output_head_v1);
};

static inline int
zwlr_output_head_v1_add_listener(struct zwlr_output_head_v1 *zwlr_output_head_v1,
				 const struct zwlr_output_head_v1_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) zwlr_output_head_v1,
				     (void (**)(void)) listener, data);
}

struct zwlr_output_mode_v1_listener {
	void (*size)(void *data,
		     struct zwlr_output_mode_v1 *zwlr_output_mode_v1,
		     int32_t width,
		     int32_t height);

	void (*refresh)(void *data,
			struct zwlr_output_mode_v1 *zwlr_output_mode_v1,
			int32_t refresh);

	void (*preferred)(void *data,
			  struct zwlr_output_mode_v1 *zwlr_output_mode_v1);

	void (*finished)(void *data,
			 struct zwlr_output_mode_v1 *zwlr_output_mode_v1);
};

static inline int
zwlr_output_mode_v1_add_listener(struct zwlr_output_mode_v1 *zwlr_output_mode_v1,
				 const struct zwlr_output_mode_v1_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) zwlr_output_mode_v1,
				     (void (**)(void)) listener, data);
}

struct zwlr_output_configuration_v1_listener {
	void (*succeeded)(void *data,
			  struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1);

	void (*failed)(void *data,
		       struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1);

	void (*cancelled)(void *data,
			  struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1);
};

static inline int
zwlr_output_configuration_v1_add_listener(struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1,
					  const struct zwlr_output_configuration_v1_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) zwlr_output_configuration_v1,
				     (void (**)(void)) listener, data);
}

int wlrdisplay(int argc, char *argv[]);

#ifdef  __cplusplus
}
#endif

#endif
