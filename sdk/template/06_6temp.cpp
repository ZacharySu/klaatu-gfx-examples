/*
Book:          Game and Graphics Programming for iOS and Android with OpenGL(R) ES 2.0
Author:        Romain Marucchi-Foino
ISBN-10:     1119975913
ISBN-13:     978-1119975915
Publisher:     John Wiley & Sons
Copyright (C) 2011 Romain Marucchi-Foino
This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of
this software. Permission is granted to anyone who either own or purchase a copy of
the book specified above, to use this software for any purpose, including commercial
applications subject to the following restrictions:
1. The origin of this software must not be misrepresented; you must not claim that
you wrote the original software. If you use this software in a product, an acknowledgment
in the product would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented
as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/
#include "templateApp.h"
#define OBJ_FILE ( char * )"pinball.obj"
#define PHYSIC_FILE ( char * )"pinball.bullet"
#define VERTEX_SHADER ( char * )"vertex.glsl"
#define FRAGMENT_SHADER ( char * )"fragment.glsl"
OBJ *obj = NULL;
PROGRAM *program = NULL;
TEMPLATEAPP templateApp = { templateAppInit,
    templateAppDraw,
    templateAppToucheBegan
};

btSoftBodyRigidBodyCollisionConfiguration *collisionconfiguration = NULL;
btCollisionDispatcher *dispatcher = NULL;
btBroadphaseInterface *broadphase = NULL;
btConstraintSolver *solver = NULL;
btSoftRigidDynamicsWorld *dynamicsworld = NULL;
void near_callback(btBroadphasePair & btbroadphasepair, btCollisionDispatcher & btdispatcher, const btDispatcherInfo & btdispatcherinfo)
{
    OBJMESH *objmesh0 = (OBJMESH *) ((btRigidBody *)
				     (btbroadphasepair.m_pProxy0->m_clientObject))->getUserPointer();
    OBJMESH *objmesh1 = (OBJMESH *) ((btRigidBody *)
				     (btbroadphasepair.m_pProxy1->m_clientObject))->getUserPointer();
    btdispatcher.defaultNearCallback(btbroadphasepair, btdispatcher, btdispatcherinfo);
}

void init_physic_world(void)
{
    collisionconfiguration = new btSoftBodyRigidBodyCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionconfiguration);
    broadphase = new btDbvtBroadphase();
    solver = new btSequentialImpulseConstraintSolver();
    dynamicsworld = new btSoftRigidDynamicsWorld(dispatcher, broadphase, solver, collisionconfiguration);
    dynamicsworld->setGravity(btVector3(0.0f, 0.0f, -9.8f));
}

void load_physic_world(void)
{
}

void free_physic_world(void)
{
    while (dynamicsworld->getNumCollisionObjects()) {
	btCollisionObject *btcollisionobject = dynamicsworld->getCollisionObjectArray()[0];
	btRigidBody *btrigidbody = btRigidBody::upcast(btcollisionobject);
	if (btrigidbody) {
	    delete btrigidbody->getCollisionShape();
	    delete btrigidbody->getMotionState();
	    dynamicsworld->removeRigidBody(btrigidbody);
	    dynamicsworld->removeCollisionObject(btcollisionobject);
	    delete btrigidbody;
	}
    }
    delete collisionconfiguration;
    collisionconfiguration = NULL;
    delete dispatcher;
    dispatcher = NULL;
    delete broadphase;
    broadphase = NULL;
    delete solver;
    solver = NULL;
    delete dynamicsworld;
    dynamicsworld = NULL;
}

void program_bind_attrib_location(void *ptr)
{
    PROGRAM *program = (PROGRAM *) ptr;
    glBindAttribLocation(program->pid, 0, "POSITION");
    glBindAttribLocation(program->pid, 2, "TEXCOORD0");
}

void load_game(void)
{
    obj = OBJ_load(OBJ_FILE, 1);
    unsigned int i = 0;
    while (i != obj->n_objmesh) {
	OBJ_build_mesh(obj, i);
	OBJ_free_mesh_vertex_data(obj, i);
	++i;
    }
    init_physic_world();
    dispatcher->setNearCallback(near_callback);
    load_physic_world();
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

void templateAppInit(int width, int height)
{
    GFX_start();
    glViewport(0.0f, 0.0f, width, height);
    GFX_set_matrix_mode(PROJECTION_MATRIX);
    GFX_load_identity();
    GFX_set_perspective(45.0f, (float)width / (float)height, 1.0f, 100.0f, 0.0f);
    load_game();
}

void templateAppDraw(void)
{
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    GFX_set_matrix_mode(MODELVIEW_MATRIX);
    GFX_load_identity(); {
	vec3 e = { 0.0f, -21.36f, 19.64f }, c = {
	0.0f, -20.36f, 19.22f}, u = {
	0.0f, 0.0f, 1.0f};
	GFX_look_at(&e, &c, &u);
    }
    unsigned int i = 0;
    while (i != obj->n_objmesh) {
	OBJMESH *objmesh = &obj->objmesh[i];
	GFX_push_matrix();
	if (objmesh->btrigidbody) {
	    mat4 mat;
	    objmesh->btrigidbody->getWorldTransform().getOpenGLMatrix((float *)&mat);
	    GFX_multiply_matrix(&mat);
	} else
	    GFX_translate(objmesh->location.x, objmesh->location.y, objmesh->location.z);
	glUniformMatrix4fv(program->uniform_array[0].location, 1, GL_FALSE, (float *)GFX_get_modelview_projection_matrix());
	OBJ_draw_mesh(obj, i);
	GFX_pop_matrix();
	++i;
    }
    dynamicsworld->stepSimulation(1.0f / 60.0f);
}

void templateAppToucheBegan(float x, float y, unsigned int tap_count)
{
}

void templateAppExit(void)
{
    free_physic_world();
    OBJ_free(obj);
}
