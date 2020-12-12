#ifndef DRM_H
#define DRM_H

#ifdef FAND_DRM_SUPPORT

int drm_init(unsigned card_idx);
int drm_close(void);
int drm_get_temp(void);


#endif /* FAND_DRM_SUPPORT */

#endif /* DRM_H */
