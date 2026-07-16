function hsv_color(hue, saturation, value)
    color = {};

    f = hue - math.floor(hue)

    if hue < 1 then
        color = {1.0, f, 0.0}
    elseif hue < 2 then
        color = {1.0 - f, 1.0, 0.0}
    elseif hue < 3 then
        color = {0.0, 1.0, f}
    elseif hue < 4 then
        color = {0.0, 1.0 - f, 1.0}
    elseif hue < 5 then
        color = {f, 0.0, 1.0}
    elseif hue < 6 then
        color = {1.0, 0.0, 1.0 - f}
    end

    for i = 1, #color do
        color[i] = (color[i] * saturation + (1.0 - saturation)) * value
    end

    return color
end

color = hsv_color(0.375, 0.65, 0.9)

print("lua called! yay")
summon_cube(20.0, 0.5, 0.5, color[1], color[2], color[3])