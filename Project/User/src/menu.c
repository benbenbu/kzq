#include "menu.h"
#include <string.h>

// 静态菜单状态变量
static MenuState menu_state;

// 计算菜单项数量的辅助函数
uint8_t Menu_CountItems(MenuItem* menu) {
    uint8_t count = 0;
    if (menu == NULL) {
        return 0;
    }
    
    while (menu[count].ch_name != NULL || menu[count].en_name != NULL) {
        count++;
    }
    
    return count;
}

// 初始化菜单系统
void Menu_Init(MenuItem* root_menu, MenuLanguage initial_language, MenuHighlightMode highlight_mode) {
    // 初始化菜单状态
    memset(&menu_state, 0, sizeof(MenuState));
    
    // 设置根菜单
    menu_state.current_menu = root_menu;
    menu_state.current_item = root_menu;
    menu_state.language = initial_language;
    menu_state.highlight_mode = highlight_mode;
    menu_state.current_index = 0;
    menu_state.item_count = Menu_CountItems(root_menu);
    
    // 设置所有菜单项的父指针
    MenuItem* item = root_menu;
    while (item->ch_name != NULL || item->en_name != NULL) {
        if (item->submenu != NULL) {
            MenuItem* subitem = item->submenu;
            while (subitem->ch_name != NULL || subitem->en_name != NULL) {
                subitem->parent = item;
                subitem++;
            }
        }
        item++;
    }
}

// 设置当前语言
void Menu_SetLanguage(MenuLanguage language) {
    menu_state.language = language;
}

// 获取当前语言
MenuLanguage Menu_GetLanguage(void) {
    return menu_state.language;
}

// 设置反显模式
void Menu_SetHighlightMode(MenuHighlightMode mode) {
    menu_state.highlight_mode = mode;
}

// 获取反显模式
MenuHighlightMode Menu_GetHighlightMode(void) {
    return menu_state.highlight_mode;
}

// 进入子菜单
void Menu_EnterSubmenu(void) {
    if (menu_state.current_item->submenu != NULL) {
        menu_state.current_menu = menu_state.current_item->submenu;
        menu_state.current_item = menu_state.current_menu;
        menu_state.current_index = 0;
        menu_state.item_count = Menu_CountItems(menu_state.current_menu);
    }
}

// 返回父菜单
void Menu_ReturnToParent(void) {
    if (menu_state.current_menu != NULL && menu_state.current_menu[0].parent != NULL) {
        // 找到父菜单
        MenuItem* parent_item = menu_state.current_menu[0].parent;
        MenuItem* parent_menu = parent_item;
        
        // 找到父菜单的起始位置
        while (parent_menu->parent != NULL) {
            parent_menu--;
        }
        
        // 更新菜单状态
        menu_state.current_menu = parent_menu;
        menu_state.current_item = parent_item;
        menu_state.item_count = Menu_CountItems(menu_state.current_menu);
        
        // 重新计算当前索引
        uint8_t index = 0;
        while (menu_state.current_menu[index].ch_name != NULL || menu_state.current_menu[index].en_name != NULL) {
            if (&menu_state.current_menu[index] == parent_item) {
                menu_state.current_index = index;
                break;
            }
            index++;
        }
    }
}

// 选择上一个菜单项
void Menu_SelectPrevious(void) {
    if (menu_state.item_count > 0) {
        if (menu_state.current_index > 0) {
            menu_state.current_index--;
        } else {
            menu_state.current_index = menu_state.item_count - 1; // 循环到最后一项
        }
        menu_state.current_item = &menu_state.current_menu[menu_state.current_index];
    }
}

// 选择下一个菜单项
void Menu_SelectNext(void) {
    if (menu_state.item_count > 0) {
        if (menu_state.current_index < menu_state.item_count - 1) {
            menu_state.current_index++;
        } else {
            menu_state.current_index = 0; // 循环到第一项
        }
        menu_state.current_item = &menu_state.current_menu[menu_state.current_index];
    }
}

// 执行当前选中菜单项的回调函数
void Menu_ExecuteCurrent(void) {
    if (menu_state.current_item != NULL && menu_state.current_item->callback != NULL) {
        menu_state.current_item->callback();
    }
}

// 获取当前菜单的菜单项数量
uint8_t Menu_GetCurrentItemCount(void) {
    return menu_state.item_count;
}

// 获取当前菜单中指定索引的菜单项
MenuItem* Menu_GetItemByIndex(uint8_t index) {
    if (index < menu_state.item_count) {
        return &menu_state.current_menu[index];
    }
    return NULL;
}

// 获取当前选中的菜单项
MenuItem* Menu_GetCurrentItem(void) {
    return menu_state.current_item;
}

// 获取当前菜单
MenuItem* Menu_GetCurrentMenu(void) {
    return menu_state.current_menu;
}
