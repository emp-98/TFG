Lum_testo = [530 580 650 620 560 600 580 550 555 560 615 315 318 315 309 318 316 312 195 199 192 201 200 201 129 124 104 113];
Lum_sensor = [276.78 297.20 339.79 354.24 285.92 325.61 301.93 261.34 288.85 290.91 332 140.84 142.70 140.59 137.77 142.30 140.95 139.42 80.26 82.27 80.02 84.23 84.15 84.22 45.72 43.49 39.29 41.79];
plot(Lum_testo, '-*')
grid on
hold on
plot(Lum_sensor, '-*')
title('Comparativa luminosidad')
legend('Testo480', 'TSL2591')
xlabel('Nº medidas')
ylabel('Luminosidad (lux)')
