/* see the file COPYRIGHT for copyright notice for this file */
#include "templateApp.h"

class KlaatuAPI : public KlaatuAPITemplate {
public:
    void init(int width, int height);
    void draw(void);
    void finish(void);
};
static KlaatuAPI current_methods;
#define OBJ_FILE ( char * )"Scene.obj"
#define VERTEX_SHADER ( char * )"vertex.glsl"
#define FRAGMENT_SHADER ( char * )"fragment.glsl"
OBJ *obj = NULL;
PROGRAM *program = NULL;
vec3 location = { 0.0f, 0.0f, 1.84f };

float rotz = 0.0f, rotx = 90.0f;
vec4 frustum[6];
float screen_size = 0.0f;
vec2 view_location, view_delta = { 0.0f, 0.0f };
vec3 move_location = { 0.0f, 0.0f, 0.0f }, move_delta;

void program_bind_attrib_location(void *ptr)
{
    PROGRAM *program = (PROGRAM *) ptr;
    glBindAttribLocation(program->pid, 0, "POSITION");
    glBindAttribLocation(program->pid, 2, "TEXCOORD0");
}

void templateAppInit(int width, int height)
{
    screen_size = (width > height) ? width : height;
    GFX_start();
    glViewport(0.0f, 0.0f, width, height);
    GFX_set_matrix_mode(PROJECTION_MATRIX);
    GFX_load_identity();
    GFX_set_perspective(80.0f, (float)width / (float)height, 0.1f, 100.0f, -90.0f);
    obj = OBJ_load(OBJ_FILE, 1);
    unsigned int i = 0;
    while (i != obj->n_objmesh) {
	OBJ_build_mesh(obj, i);
	OBJ_free_mesh_vertex_data(obj, i);
	++i;
    }
    i = 0;
    while (i != obj->n_texture) {
	OBJ_build_texture(obj, i, obj->texture_path, TEXTURE_MIPMAP | TEXTURE_16_BITS, TEXTURE_FILTER_2X, 0.0f);
	++i;
    }
    i = 0;
    while (i != obj->n_objmaterial) {
	OBJ_build_material(obj, i, NULL);
	++i;
    }
    program = PROGRAM_create((char *)"default", VERTEX_SHADER, FRAGMENT_SHADER, 1, 0, program_bind_attrib_location, NULL);
    PROGRAM_draw(program);
    glUniform1i(PROGRAM_get_uniform_location(program, (char *)"DIFFUSE"), 1);
}

void templateAppDraw(void)
{
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    GFX_set_matrix_mode(MODELVIEW_MATRIX);
    GFX_load_identity();
    if (view_delta.x || view_delta.y) {
	if (view_delta.y)
	    rotz -= view_delta.y;
	if (view_delta.x) {
	    rotx += view_delta.x;
	    rotx = CLAMP(rotx, 0.0f, 180.0f);
	}
	view_delta.x = view_delta.y = 0.0f;
    }
    if (move_delta.z) {
	vec3 forward;
	float r = rotz * DEG_TO_RAD, c = cosf(r), s = sinf(r);
	forward.x = c * move_delta.y - s * move_delta.x;
	forward.y = s * move_delta.y + c * move_delta.x;
	location.x += forward.x * move_delta.z * 0.1f;
	location.y += forward.y * move_delta.z * 0.1f;
	forward.z = sinf((rotx - 90.0f) * DEG_TO_RAD);
	if (move_delta.x < -0.99f)
	    location.z -= forward.z * move_delta.z * 0.1f;
	else if (move_delta.x > 0.99f)
	    location.z += forward.z * move_delta.z * 0.1f;
    }
    GFX_translate(location.x, location.y, location.z);
    GFX_rotate(rotz, 0.0f, 0.0f, 1.0f);
    GFX_rotate(rotx, 1.0f, 0.0f, 0.0f);
    mat4_invert(GFX_get_modelview_matrix());
    build_frustum(frustum, GFX_get_modelview_matrix(), GFX_get_projection_matrix());
    unsigned int i = 0;
    while (i != obj->n_objmesh) {
	OBJMESH *objmesh = &obj->objmesh[i];
	objmesh->distance = sphere_distance_in_frustum(frustum, &objmesh->location, objmesh->radius);
	if (objmesh->distance > 0.0f) {
	    GFX_push_matrix();
	    GFX_translate(objmesh->location.x, objmesh->location.y, objmesh->location.z);
	    glUniformMatrix4fv(program->uniform_array[0].location, 1, GL_FALSE, (float *)GFX_get_modelview_projection_matrix());
	    OBJ_draw_mesh(obj, i);
	    GFX_pop_matrix();
	}
	++i;
    }
}

void templateAppToucheBegan(float x, float y, unsigned int tap_count)
{
    if (y < (screen_size * 0.5f)) {
	move_location.x = x;
	move_location.y = y;
    } else {
	view_location.x = x;
	view_location.y = y;
    }
}

void templateAppToucheMoved(float x, float y, unsigned int tap_count)
{
    if (y > ((screen_size * 0.5f) - (screen_size * 0.05f)) && y < ((screen_size * 0.5f) + (screen_size * 0.05f))) {
	move_delta.z = view_delta.x = view_delta.y = 0.0f;
	move_location.x = x;
	move_location.y = y;
	view_location.x = x;
	view_location.y = y;
    } else if (y < (screen_size * 0.5f)) {
	vec3 touche = { x,
	    y,
	    0.0f
	};
	vec3_diff(&move_delta, &touche, &move_location);
	vec3_normalize(&move_delta, &move_delta);
	move_delta.z = CLAMP(vec3_dist(&move_location, &touche) / 128.0f, 0.0f, 1.0f);
    } else {
	view_delta.x = view_delta.x * 0.75f + (x - view_location.x) * 0.25f;
	view_delta.y = view_delta.y * 0.75f + (y - view_location.y) * 0.25f;
	view_location.x = x;
	view_location.y = y;
    }
}

void templateAppToucheEnded(float x, float y, unsigned int tap_count)
{
    move_delta.z = 0.0f;
}

void templateAppExit(void)
{
    OBJ_free(obj);
}
int main(int argc, char *argv[])
{
    return klaatu_main(argc, argv);
}
