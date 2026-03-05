#ifndef LCD_ROTATION_H
#define LCD_ROTATION_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Set LCD rotation at runtime (all 3 screens).
 * @param degrees 0, 90, 180, or 270
 * @return 0 on success, -1 on invalid input
 */
int lcd_set_rotation(int degrees);

/**
 * Get current LCD rotation.
 * @return 0, 90, 180, or 270
 */
int lcd_get_rotation(void);

#ifdef __cplusplus
}
#endif

#endif /* LCD_ROTATION_H */
