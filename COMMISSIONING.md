# Commissioning Guide — ESP32-C6 Matter Fan

## Device Info

| Param | Value |
|-------|-------|
| Endpoint | Dimmable Light (0x0100) |
| VID | 65521 (0xFFF1) |
| PID | 32768 (0x8000) |
| Discriminator | 3840 (0xF00) |
| Setup PIN | 20202021 |
| Manual Pairing Code | **34970112332** |

## Home Assistant Setup

### Hardware

| Device | IP | MAC | Notes |
|--------|-----|-----|-------|
| HA Server | 192.168.10.15 | - | Tiny PC 1135G7, Ubuntu |
| SLZB-06M | 192.168.10.18 | 68:25:dd:48:04:4b | Thread BR, port 6638 |
| ESP32-C6 #1 | - | ac:eb:e6:c1:8a:c8 | COM4 |
| ESP32-C6 #2 | - | ac:eb:e6:c1:c7:6c | COM5 |

### HA Network
- Backbone interface: `enp88s0`
- HA Bluetooth: `64:79:F0:45:79:B3` (ID: 0)
- Thread network: `ha-thread-9a87`, ULA: `fd63:1325:6ef9::/48`

### Thread Dataset
```
0e080000000000010000000300000f4a0300001435060004001fffe002089971e7e7b46f8daa0708fd858aab07fb1693051082bd2483f5db9526238b2a7ec3b7fc4c030e68612d7468726561642d3961383701029a8704104155fe8a5113e281b9fd3d6e91b6ddb60c0402a0f7f8
```

## HA Add-on Configuration

### OpenThread Border Router (v2.16.7)
- Network Device: `192.168.10.18:6638`
- Baudrate: 460800
- Hardware flow control: OFF
- Auto flash firmware: OFF
- Backbone Network Interface: `enp88s0`

### Matter Server (v8.4.0)
- Bluetooth Adapter ID: `0`

## Commissioning Steps

1. SLZB-06M: Mode → "Thread to remote OTBR"
2. HA: Install Matter Server + OTBR add-ons
3. OTBR config: Network Device = `192.168.10.18:6638`, Backbone = `enp88s0`
4. Matter Server config: Bluetooth Adapter ID = `0`
5. HA: Add Bluetooth integration
6. Flash ESP32-C6 with firmware
7. Erase NVS: `esptool.py -p COM4 erase_region 0x9000 0x6000`
8. Matter Server Web UI → Commission → `34970112332`
9. ESP32-C6 must be within BLE range of HA machine (< 5m)

## Troubleshooting

| Issue | Fix |
|-------|-----|
| "No update available" | Commission first, then check |
| Commission fails | Erase NVS, retry. Keep ESP close to HA. |
| Device not found via BLE | Check HA Bluetooth integration is running |
| Web UI SLZB-06M down | SSH: `ssh root@192.168.10.18`, restart nginx |
