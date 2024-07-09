uniform sampler2D texture;

varying vec2 uv;
varying vec3 modelNormal;

void main()
{
    // lookup the pixel in the texture
    vec4 pixel = texture2D(texture, uv);

    // multiply it by the color
    gl_FragColor = vec4(modelNormal,1);//gl_Color * pixel;
}