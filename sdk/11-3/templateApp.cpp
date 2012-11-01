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
OBJ *obj = NULL;

OBJMESH *objmesh = NULL;
int viewport_matrix[4];
LIGHT *light = NULL;
vec3 center = { 0.0f, 0.0f, 0.0f }, up_axis = {
0.0f, 0.0f, 1.0f};

mat4 projector_matrix;
int main_buffer;
unsigned int depth_texture, shadowmap_buffer, shadowmap_width = 128, shadowmap_height = 256;
void program_bind_attrib_location(void *ptr)
{
    PROGRAM *program = (PROGRAM *) ptr;
    glBindAttribLocation(program->pid, 0, "POSITION");
    glBindAttribLocation(program->pid, 1, "NORMAL");
    glBindAttribLocation(program->pid, 2, "TEXCOORD0");
    glBindAttribLocation(program->pid, 3, "TANGENT0");
}

void program_draw(void *ptr)
{
    unsigned int i = 0;
    PROGRAM *program = (PROGRAM *) ptr;
    while (i != program->uniform_count) {
	if (program->uniform_array[i].constant) {
	    ++i;
	    continue;
	} else if (!strcmp(program->uniform_array[i].name, "MODELVIEWPROJECTIONMATRIX")) {
	    glUniformMatrix4fv(program->uniform_array[i].location, 1, GL_FALSE, (float *)GFX_get_modelview_projection_matrix());
	} else if (!strcmp(program->uniform_array[i].name, "PROJECTOR")) {
	    glUniform1i(program->uniform_array[i].location, 0);
	    program->uniform_array[i].constant = 1;
	} else if (!strcmp(program->uniform_array[i].name, "DIFFUSE")) {
	    glUniform1i(program->uniform_array[i].location, 1);
	    program->uniform_array[i].constant = 1;
	} else if (!strcmp(program->uniform_array[i].name, "BUMP")) {
	    glUniform1i(program->uniform_array[i].location, 4);
	    program->uniform_array[i].constant = 1;
	}
	// Matrix Data
	else if (!strcmp(program->uniform_array[i].name, "MODELVIEWMATRIX")) {
	    glUniformMatrix4fv(program->uniform_array[i].location, 1, GL_FALSE, (float *)GFX_get_modelview_matrix());
	} else if (!strcmp(program->uniform_array[i].name, "PROJECTIONMATRIX")) {
	    glUniformMatrix4fv(program->uniform_array[i].location, 1, GL_FALSE, (float *)GFX_get_projection_matrix());
	    program->uniform_array[i].constant = 1;
	} else if (!strcmp(program->uniform_array[i].name, "NORMALMATRIX")) {
	    glUniformMatrix3fv(program->uniform_array[i].location, 1, GL_FALSE, (float *)GFX_get_normal_matrix());
	} else if (!strcmp(program->uniform_array[i].name, "PROJECTORMATRIX")) {
	    glUniformMatrix4fv(program->uniform_array[i].location, 1, GL_FALSE, (float *)&projector_matrix);
	}
	// Material Data
	else if (!strcmp(program->uniform_array[i].name, "MATERIAL.ambient")) {
	    glUniform4fv(program->uniform_array[i].location, 1, (float *)&objmesh->current_material->ambient);
	    program->uniform_array[i].constant = 1;
	} else if (!strcmp(program->uniform_array[i].name, "MATERIAL.diffuse")) {
	    glUniform4fv(program->uniform_array[i].location, 1, (float *)&objmesh->current_material->diffuse);
	    program->uniform_array[i].constant = 1;
	} else if (!strcmp(program->uniform_array[i].name, "MATERIAL.specular")) {
	    glUniform4fv(program->uniform_array[i].location, 1, (float *)&objmesh->current_material->specular);
	    program->uniform_array[i].constant = 1;
	} else if (!strcmp(program->uniform_array[i].name, "MATERIAL.shininess")) {
	    glUniform1f(program->uniform_array[i].location, objmesh->current_material->specular_exponent * 0.128f);
	    program->uniform_array[i].constant = 1;
	}
	// Light Data
	else if (!strcmp(program->uniform_array[i].name, "LIGHT_FS.color")) {
	    glUniform4fv(program->uniform_array[i].location, 1, (float *)&light->color);
	    program->uniform_array[i].constant = 1;
	} else if (!strcmp(program->uniform_array[i].name, "LIGHT_VS.position")) {
	    vec4 position;
	    static float rot_angle = 0.0f;
	    light->position.x = 7.5f * cosf(rot_angle * DEG_TO_RAD);
	    light->position.y = 7.5f * sinf(rot_angle * DEG_TO_RAD);
	    rot_angle += 0.25f;
	    LIGHT_get_position_in_eye_space(light, &gfx.modelview_matrix[gfx.modelview_matrix_index - 1], &position);
	    glUniform3fv(program->uniform_array[i].location, 1, (float *)&position);
	} else if (!strcmp(program->uniform_array[i].name, "LIGHT_VS.spot_direction")) {
	    vec3 direction;
	    vec3_diff(&light->spot_direction, &center, (vec3 *) & light->position);
	    vec3_normalize(&light->spot_direction, &light->spot_direction);
	    LIGHT_get_direction_in_object_space(light, &gfx.modelview_matrix[gfx.modelview_matrix_index - 1], &direction);
	    glUniform3fv(program->uniform_array[i].location, 1, (float *)&direction);
	} else if (!strcmp(program->uniform_array[i].name, "LIGHT_FS.spot_cos_cutoff")) {
	    glUniform1f(program->uniform_array[i].location, light->spot_cos_cutoff);
	    program->uniform_array[i].constant = 1;
	} else if (!strcmp(program->uniform_array[i].name, "LIGHT_FS.spot_blend")) {
	    glUniform1f(program->uniform_array[i].location, light->spot_blend);
	    program->uniform_array[i].constant = 1;
	}
	++i;
    }
}

void templateAppInit(int width, int height)
{
    glViewport(0.0f, 0.0f, width, height);
    glGetIntegerv(GL_VIEWPORT, viewport_matrix);
    GFX_start();
    obj = OBJ_load(OBJ_FILE, 1);
    unsigned int i = 0;
    while (i != obj->n_objmesh) {
	OBJ_optimize_mesh(obj, i, 128);
	OBJ_build_mesh2(obj, i);
	OBJ_free_mesh_vertex_data(obj, i);
	++i;
    }
    i = 0;
    while (i != obj->n_texture) {
	OBJ_build_texture(obj, i, obj->texture_path, TEXTURE_MIPMAP | TEXTURE_16_BITS, TEXTURE_FILTER_2X, 0.0f);
	++i;
    }
    i = 0;
    while (i != obj->n_program) {
	OBJ_build_program(obj, i, program_bind_attrib_location, program_draw, 1, obj->program_path);
	++i;
    }
    i = 0;
    while (i != obj->n_objmaterial) {
	OBJ_build_material(obj, i, NULL);
	++i;
    }
    vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    vec3 position = { 7.5f, 0.0f, 6.0f };
    light = LIGHT_create_spot((char *)"spot", &color, &position, 0.0f, 0.0f, 0.0f, 75.0f, 0.05f);
    OBJ_get_mesh(obj, (char *)"projector", 0)->visible = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &main_buffer);
    if (main_buffer == -1)
	main_buffer = 0;
    glGenFramebuffers(1, &shadowmap_buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowmap_buffer);
    glGenTextures(1, &depth_texture);
    glBindTexture(GL_TEXTURE_2D, depth_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowmap_width, shadowmap_height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_texture, 0);
}

void draw_scene_from_projector(void)
{
    GFX_set_matrix_mode(PROJECTION_MATRIX);
    GFX_load_identity();
    GFX_set_perspective(light->spot_fov, (float)viewport_matrix[2] / (float)viewport_matrix[3], 1.0f, 20.0f, -90.0f);
    GFX_set_matrix_mode(MODELVIEW_MATRIX);
    GFX_load_identity();
    GFX_look_at((vec3 *) & light->position, &center, &up_axis);
    projector_matrix.m[0].x = 0.5f;
    projector_matrix.m[0].y = 0.0f;
    projector_matrix.m[0].z = 0.0f;
    projector_matrix.m[0].w = 0.0f;
    projector_matrix.m[1].x = 0.0f;
    projector_matrix.m[1].y = 0.5f;
    projector_matrix.m[1].z = 0.0f;
    projector_matrix.m[1].w = 0.0f;
    projector_matrix.m[2].x = 0.0f;
    projector_matrix.m[2].y = 0.0f;
    projector_matrix.m[2].z = 0.5f;
    projector_matrix.m[2].w = 0.0f;
    projector_matrix.m[3].x = 0.5f;
    projector_matrix.m[3].y = 0.5f;
    projector_matrix.m[3].z = 0.5f;
    projector_matrix.m[3].w = 1.0f;
    mat4_multiply_mat4(&projector_matrix, &projector_matrix, GFX_get_modelview_projection_matrix());
    glBindFramebuffer(GL_FRAMEBUFFER, shadowmap_buffer);
    glViewport(0, 0, shadowmap_width, shadowmap_height);
    glClear(GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_FRONT);
    unsigned int i = 0;
    PROGRAM *program = OBJ_get_program(obj, "writedepth", 0);
    while (i != obj->n_objmaterial) {
	obj->objmaterial[i].program = program;
	++i;
    }
    i = 0;
    while (i != obj->n_objmesh) {
	objmesh = &obj->objmesh[i];
	GFX_push_matrix();
	GFX_translate(objmesh->location.x, objmesh->location.y, objmesh->location.z);
	OBJ_draw_mesh(obj, i);
	GFX_pop_matrix();
	++i;
    }
    glCullFace(GL_BACK);
}

void draw_scene(void)
{
    glBindFramebuffer(GL_FRAMEBUFFER, main_buffer);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, viewport_matrix[2], viewport_matrix[3]);
    GFX_set_matrix_mode(PROJECTION_MATRIX);
    GFX_load_identity();
    GFX_set_perspective(45.0f, (float)viewport_matrix[2] / (float)viewport_matrix[3], 0.1f, 100.0f, -90.0f);
    GFX_set_matrix_mode(MODELVIEW_MATRIX);
    GFX_load_identity();
    GFX_translate(14.0f, -12.0f, 7.0f);
    GFX_rotate(48.5f, 0.0f, 0.0f, 1.0f);
    GFX_rotate(72.0, 1.0f, 0.0f, 0.0f);
    mat4_invert(GFX_get_modelview_matrix());
    mat4 projector_matrix_copy;
    mat4_copy_mat4(&projector_matrix_copy, &projector_matrix);
    unsigned int i = 0;
    PROGRAM *program = OBJ_get_program(obj, "lighting", 0);
    while (i != obj->n_objmaterial) {
	obj->objmaterial[i].program = program;
	++i;
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depth_texture);
    i = 0;
    while (i != obj->n_objmesh) {
	objmesh = &obj->objmesh[i];
	GFX_push_matrix();
	GFX_translate(objmesh->location.x, objmesh->location.y, objmesh->location.z);
	mat4_copy_mat4(&projector_matrix, &projector_matrix_copy);
	mat4_translate(&projector_matrix, &projector_matrix, &objmesh->location);
	OBJ_draw_mesh(obj, i);
	GFX_pop_matrix();
	++i;
    }
}

void templateAppDraw(void)
{
    draw_scene_from_projector();
    draw_scene();
}

void templateAppExit(void)
{
    light = LIGHT_free(light);
    OBJ_free(obj);
}
int main(int argc, char *argv[])
{
    return klaatu_main(argc, argv);
}
