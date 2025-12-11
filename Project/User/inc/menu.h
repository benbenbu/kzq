#ifndef __MENU_H__
#define __MENU_H__

#include <stdint.h>

// 语言类型定义
typedef enum {
    LANGUAGE_CHINESE,
    LANGUAGE_ENGLISH
} MenuLanguage;

// 菜单反显模式
typedef enum {
    HIGHLIGHT_NONE,
    HIGHLIGHT_CURRENT,
    HIGHLIGHT_ALL
} MenuHighlightMode;

// 菜单项结构体定义
typedef struct MenuItem {
    const char* ch_name;    // 中文名称
    const char* en_name;    // 英文名称
    struct MenuItem* submenu; // 子菜单指针
    void (*callback)(void);  // 选中时的回调函数
    struct MenuItem* parent; // 父菜单指针
    uint8_t id;             // 菜单项ID
} MenuItem;

// 菜单状态结构体
typedef struct {
    MenuItem* current_menu;  // 当前菜单
    MenuItem* current_item;  // 当前选中的菜单项
    MenuLanguage language;   // 当前语言
    MenuHighlightMode highlight_mode; // 反显模式
    uint8_t current_index;   // 当前选中项索引
    uint8_t item_count;      // 当前菜单的菜单项数量
} MenuState;

// 函数声明

// 初始化菜单系统
void Menu_Init(MenuItem* root_menu, MenuLanguage initial_language, MenuHighlightMode highlight_mode);

// 设置当前语言
void Menu_SetLanguage(MenuLanguage language);

// 获取当前语言
MenuLanguage Menu_GetLanguage(void);

// 设置反显模式
void Menu_SetHighlightMode(MenuHighlightMode mode);

// 获取反显模式
MenuHighlightMode Menu_GetHighlightMode(void);

// 进入子菜单
void Menu_EnterSubmenu(void);

// 返回父菜单
void Menu_ReturnToParent(void);

// 选择上一个菜单项
void Menu_SelectPrevious(void);

// 选择下一个菜单项
void Menu_SelectNext(void);

// 执行当前选中菜单项的回调函数
void Menu_ExecuteCurrent(void);

// 获取当前菜单的菜单项数量
uint8_t Menu_GetCurrentItemCount(void);

// 获取当前菜单中指定索引的菜单项
MenuItem* Menu_GetItemByIndex(uint8_t index);

// 获取当前选中的菜单项
MenuItem* Menu_GetCurrentItem(void);

// 获取当前菜单
MenuItem* Menu_GetCurrentMenu(void);

// 计算菜单项数量的辅助函数
uint8_t Menu_CountItems(MenuItem* menu);

#endif /* __MENU_H__ */