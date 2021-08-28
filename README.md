ventctl
==

:warning: Проект не завершен

При развертывании проекта с гитхаба после установки mbed необходимо поправить `framework-mbed/features/netsocket/emac-drivers/TARGET_STM/stm32xx_emac.cpp`, заменить на строчке 46:

```cpp
    #define ETH_ARCH_PHY_ADDRESS    (0x00)
```

на

```cpp
    #ifndef ETH_ARCH_PHY_ADDRESS
        #define ETH_ARCH_PHY_ADDRESS    (0x01)
    #endif
```

