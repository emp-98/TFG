HR_g = [33 32.8 32.7 32.4 32.2 32 31.8 31.6 31.4 31.2 31.1 30.8 30.7 30.5 30.4 30.3 30.2 30.1 30 30 30 30 29.9 30 29.9 30 33.7 33.8 33.8 33.8 33.9 33.8 33.9 33.9 33.9 33.9 33.9 33.9 33.9 36 36.3 36.6 36.8 37 37.2 37.4 37.6 37.8 37.9 38.1 38.2 38.3];
plot(HR_g, '-*')
grid on
title('Humedad relativa en interior de la semiesfera')
legend('DHT22 interno')
xlabel('Nº medidas')
ylabel('Humedad relativa de globo (%)')