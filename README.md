# Clover

A custom 2D renderer built from scratch in C++ with DirectX 11, developed as a graphics programming portfolio project.

## Features

- Custom DirectX 11 rendering pipeline (device, swap chain, orthographic projection)
- Sprite batching with dynamic vertex buffers
- Texture atlasing with edge padding to eliminate sampling artifacts
- HLSL shader system with runtime hot-reload
- Framebuffer-based post-processing pipeline
- Point and directional lighting with real-time shadows
- Entity-Component-System architecture (EnTT)
- ImGui debug overlay

## Tech Stack

- **Language:** C++
- **Graphics API:** DirectX 11
- **ECS:** [EnTT](https://github.com/skypjack/entt)
- **Texture tooling:** DirectXTex, stb_rect_pack
- **Debug UI:** Dear ImGui

## Screenshots

*(add a GIF or screenshot here — the light accumulation buffer looks great and is worth leading with)*