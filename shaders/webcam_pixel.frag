// BabyKeySmash webcam option : chunky dot mosaic of the camera image ,
// the dots swell with the music ( iChannel0 = webcam , iChannel1 = audio )

void mainImage(out vec4 fragColor,in vec2 fragCoord)
{
    float bass = texture(iChannel1,vec2(0.05,0.25)).x;

    // mosaic cell count breathes slowly and pumps with the bass
    float cells = 48.0 - 14.0*bass + 6.0*sin(iTime*0.3);
    vec2 aspect = vec2(iResolution.x/iResolution.y,1.0);
    vec2 uv = fragCoord/iResolution.xy;
    vec2 grid = uv*aspect*cells;
    vec2 cell = floor(grid);

    vec3 cam = texture(iChannel0,cell/(aspect*cells)+0.5/(aspect*cells)).rgb;

    // saturate the sampled color a bit so it stays cartoonish
    float luma = dot(cam,vec3(0.299,0.587,0.114));
    cam = clamp(luma + (cam-luma)*1.6,0.0,1.0);

    // round dots , radius follows cell brightness , kept bright and cheerful
    vec2 inCell = fract(grid)-0.5;
    float radius = 0.25 + 0.30*luma + 0.1*bass;
    float dot = smoothstep(radius,radius-0.08,length(inCell));

    // colorful canvas between the dots so the picture never goes muddy
    vec3 canvas = 0.35 + 0.25*cos(iTime*0.5 + cell.xyx*0.3 + vec3(0.0,2.0,4.0));

    fragColor = vec4(mix(canvas,cam*1.3,dot),1.0);
}
