#pragma once

// Geometrie circulaire partagee entre le firmware ESP32-S3 et le simulateur
// PC. Aucune dependance materielle ici -- uniquement des constantes.
namespace CircleLayout {
constexpr int kScreenWidth = 466;
constexpr int kScreenHeight = 466;
constexpr int kCenterX = kScreenWidth / 2;   // 233
constexpr int kCenterY = kScreenHeight / 2;  // 233
constexpr int kOuterRadius = 233;
// Rien d'important ne doit depasser ce rayon (zone de securite circulaire).
constexpr int kSafeRadius = 200;
// Marge horizontale/verticale sure pour les widgets rectangulaires (evite
// les coins qui sortiraient du cercle a kSafeRadius).
constexpr int kSafeInsetPx = 233 - 200;  // 33px
}  // namespace CircleLayout
