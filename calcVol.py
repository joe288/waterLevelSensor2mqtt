import math

L =2.64        #lenth of the tank
r = 0.6945     #radius of the tank
measure = 153/100

def lyingCyl(hight):
    if hight > r*2:
        hight = hight-r*2
        volume= maxVolume
    else: volume=0

    a = pow(r,2) * L
    test= (r - hight) / r
    b = math.acos(test)
    c = r - hight
    d = math.sqrt(2 * r * hight - pow(hight,2))
    e = pow(r,2)
    volume += a * (b - c * d / e); #m³
    return volume
maxVolume = lyingCyl(r*2)
print((lyingCyl(measure)/maxVolume)*100)
print(lyingCyl(measure)*1000)
