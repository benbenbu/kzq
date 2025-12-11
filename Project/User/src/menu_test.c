#include "menu.h"
#include <stdio.h>

// 测试回调函数
void TestCallback1(void) {
    printf("执行菜单项1的回调函数\n");
}

void TestCallback2(void) {
    printf("执行菜单项2的回调函数\n");
}

void TestCallback3(void) {
    printf("执行菜单项3的回调函数\n");
}

void TestCallback4(void) {
    printf("执行菜单项4的回调函数\n");
}

void TestCallback5(void) {
    printf("执行菜单项5的回调函数\n");
}

// 定义子菜单1
MenuItem submenu1[] = {
    {"子菜单项1-1", "Submenu Item 1-1", NULL, TestCallback1, NULL, 11},
    {"子菜单项1-2", "Submenu Item 1-2", NULL, TestCallback2, NULL, 12},
    {NULL, NULL, NULL, NULL, NULL, 0} // 结束标志
};

// 定义子菜单2
MenuItem submenu2[] = {
    {"子菜单项2-1", "Submenu Item 2-1", NULL, TestCallback3, NULL, 21},
    {"子菜单项2-2", "Submenu Item 2-2", NULL, TestCallback4, NULL, 22},
    {"子菜单项2-3", "Submenu Item 2-3", NULL, TestCallback5, NULL, 23},
    {NULL, NULL, NULL, NULL, NULL, 0} // 结束标志
};

// 定义根菜单
MenuItem root_menu[] = {
    {"主菜单项1", "Main Menu Item 1", submenu1, NULL, NULL, 1},
    {"主菜单项2", "Main Menu Item 2", submenu2, NULL, NULL, 2},
    {"主菜单项3", "Main Menu Item 3", NULL, TestCallback3, NULL, 3},
    {NULL, NULL, NULL, NULL, NULL, 0} // 结束标志
};

// 显示当前菜单状态
void DisplayMenuStatus(void) {
    printf("\n===== 菜单状态 =====\n");
    printf("当前语言: %s\n", Menu_GetLanguage() == LANGUAGE_CHINESE ? "中文" : "English");
    printf("反显模式: ");
    switch (Menu_GetHighlightMode()) {
        case HIGHLIGHT_NONE:
            printf("无反显\n");
            break;
        case HIGHLIGHT_CURRENT:
            printf("当前项反显\n");
            break;
        case HIGHLIGHT_ALL:
            printf("全部反显\n");
            break;
    }
    printf("当前菜单项数量: %d\n", Menu_GetCurrentItemCount());
    printf("当前选中项索引: %d\n", menu_state.current_index);
    printf("当前选中项: %s\n", Menu_GetLanguage() == LANGUAGE_CHINESE ? 
           Menu_GetCurrentItem()->ch_name : Menu_GetCurrentItem()->en_name);
}

// 显示当前菜单内容
void DisplayCurrentMenu(void) {
    printf("\n===== 当前菜单 =====\n");
    uint8_t count = Menu_GetCurrentItemCount();
    MenuItem* item;
    
    for (uint8_t i = 0; i < count; i++) {
        item = Menu_GetItemByIndex(i);
        if (i == menu_state.current_index) {
            // 反显当前选中项
            printf("-> ");
            if (Menu_GetHighlightMode() == HIGHLIGHT_CURRENT || Menu_GetHighlightMode() == HIGHLIGHT_ALL) {
                printf("【");
            }
        } else {
            printf("   ");
        }
        
        // 显示菜单项名称
        printf("%s", Menu_GetLanguage() == LANGUAGE_CHINESE ? item->ch_name : item->en_name);
        
        // 显示子菜单标记
        if (item->submenu != NULL) {
            printf(" >");
        }
        
        // 结束反显标记
        if (i == menu_state.current_index && 
            (Menu_GetHighlightMode() == HIGHLIGHT_CURRENT || Menu_GetHighlightMode() == HIGHLIGHT_ALL)) {
            printf("】");
        }
        
        printf("\n");
    }
}

// 测试菜单功能
int main(void) {
    printf("多级菜单测试程序\n");
    printf("====================\n");
    
    // 初始化菜单系统
    Menu_Init(root_menu, LANGUAGE_CHINESE, HIGHLIGHT_CURRENT);
    
    // 显示初始菜单
    DisplayMenuStatus();
    DisplayCurrentMenu();
    
    // 测试导航
    printf("\n=== 测试导航功能 ===\n");
    printf("向下选择...\n");
    Menu_SelectNext();
    DisplayCurrentMenu();
    
    printf("\n向下选择...\n");
    Menu_SelectNext();
    DisplayCurrentMenu();
    
    printf("\n向上选择...\n");
    Menu_SelectPrevious();
    DisplayCurrentMenu();
    
    // 测试进入子菜单
    printf("\n=== 测试进入子菜单 ===\n");
    Menu_EnterSubmenu();
    DisplayMenuStatus();
    DisplayCurrentMenu();
    
    // 测试子菜单导航
    printf("\n=== 测试子菜单导航 ===\n");
    Menu_SelectNext();
    DisplayCurrentMenu();
    
    // 测试返回父菜单
    printf("\n=== 测试返回父菜单 ===\n");
    Menu_ReturnToParent();
    DisplayMenuStatus();
    DisplayCurrentMenu();
    
    // 测试语言切换
    printf("\n=== 测试语言切换 ===\n");
    Menu_SetLanguage(LANGUAGE_ENGLISH);
    DisplayMenuStatus();
    DisplayCurrentMenu();
    
    // 测试反显模式切换
    printf("\n=== 测试反显模式切换 ===\n");
    Menu_SetHighlightMode(HIGHLIGHT_ALL);
    DisplayMenuStatus();
    DisplayCurrentMenu();
    
    // 测试执行回调函数
    printf("\n=== 测试执行回调函数 ===\n");
    Menu_ExecuteCurrent();
    
    // 测试回到中文
    Menu_SetLanguage(LANGUAGE_CHINESE);
    Menu_SetHighlightMode(HIGHLIGHT_CURRENT);
    
    printf("\n=== 测试完成 ===\n");
    
    return 0;
}
