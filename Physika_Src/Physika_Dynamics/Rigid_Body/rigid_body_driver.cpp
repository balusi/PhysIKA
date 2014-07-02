/*
 * @file rigid_body_driver.cpp
 * @Basic rigid body driver class.
 * @author Tianxiang Zhang
 * 
 * This file is part of Physika, a versatile physics simulation library.
 * Copyright (C) 2013 Physika Group.
 *
 * This Source Code Form is subject to the terms of the GNU General Public License v2.0. 
 * If a copy of the GPL was not distributed with this file, you can obtain one at:
 * http://www.gnu.org/licenses/gpl-2.0.html
 *
 */

#include "Physika_Dynamics/Rigid_Body/rigid_body.h"
#include "Physika_Dynamics/Rigid_Body/rigid_body_driver.h"
#include "Physika_Geometry/Bounding_Volume/object_bvh.h"
#include "Physika_Dynamics/Collidable_Objects/mesh_based_collidable_object.h"
#include "Physika_Core/Vectors/vector_3d.h"
#include "Physika_Geometry/Surface_Mesh/surface_mesh.h"
#include "Physika_Core/Utilities/math_utilities.h"
#include "Physika_Dynamics/Collidable_Objects/collision_detection_result.h"
#include "Physika_Dynamics/Rigid_Body/rigid_driver_plugin.h"

namespace Physika{

template <typename Scalar,int Dim>
RigidBodyArchive<Scalar, Dim>::RigidBodyArchive():
	index_(0),
	rigid_body_(NULL),
	collide_object_(NULL),
	object_bvh_(NULL)
{

}

template <typename Scalar,int Dim>
RigidBodyArchive<Scalar, Dim>::RigidBodyArchive(RigidBody<Scalar, Dim>* rigid_body)
{
	setRigidBody(rigid_body);
}

template <typename Scalar,int Dim>
RigidBodyArchive<Scalar, Dim>::~RigidBodyArchive()
{
	delete collide_object_;
	delete object_bvh_;
}

template <typename Scalar,int Dim>
void RigidBodyArchive<Scalar, Dim>::setRigidBody(RigidBody<Scalar, Dim>* rigid_body)
{
	if(rigid_body == NULL)
		return;

	rigid_body_ = rigid_body;

	switch(rigid_body->objectType())
	{
		case CollidableObject<Scalar, Dim>::MESH_BASED: collide_object_ = new MeshBasedCollidableObject<Scalar, Dim>();break;
		default: std::cerr<<"Object type error!"<<std::endl; return;
	}
	MeshBasedCollidableObject<Scalar, Dim>* mesh_object = dynamic_cast<MeshBasedCollidableObject<Scalar, Dim>*>(collide_object_);
	mesh_object->setMesh(rigid_body->mesh());
	mesh_object->setTransform(rigid_body->transformPtr());

	object_bvh_ = new ObjectBVH<Scalar, Dim>();
	object_bvh_->setCollidableObject(collide_object_);

}

template <typename Scalar,int Dim>
unsigned int RigidBodyArchive<Scalar, Dim>::index() const
{
	return index_;
}

template <typename Scalar,int Dim>
void RigidBodyArchive<Scalar, Dim>::setIndex(unsigned int index)
{
	index_ = index;
}

template <typename Scalar,int Dim>
RigidBody<Scalar, Dim>* RigidBodyArchive<Scalar, Dim>::rigidBody()
{
	return rigid_body_;
}

template <typename Scalar,int Dim>
CollidableObject<Scalar, Dim>* RigidBodyArchive<Scalar, Dim>::collideObject()
{
	return collide_object_;
}

template <typename Scalar,int Dim>
ObjectBVH<Scalar, Dim>* RigidBodyArchive<Scalar, Dim>::objectBVH()
{
	return object_bvh_;
}


///////////////////////////////////////////////////////////////////////////////////////


template <typename Scalar,int Dim>
RigidBodyDriver<Scalar, Dim>::RigidBodyDriver():
	scene_bvh_()
{

}

template <typename Scalar,int Dim>
RigidBodyDriver<Scalar, Dim>::~RigidBodyDriver()
{
	unsigned int num_rigid_body = numRigidBody();
	for(unsigned int i = 0; i < num_rigid_body; ++i)
	{
		delete rigid_body_archives_[i];
	}
	rigid_body_archives_.clear();
}

template <typename Scalar,int Dim>
void RigidBodyDriver<Scalar, Dim>::run()
{
	unsigned int plugin_num = static_cast<unsigned int>((this->plugins_).size());
	RigidDriverPlugin<Scalar, Dim>* plugin;
	for(unsigned int i = 0; i < plugin_num; ++i)
	{
		plugin = dynamic_cast<RigidDriverPlugin<Scalar, Dim>*>((this->plugins_)[i]);
		if(plugin != NULL)
			plugin->onRun();
	}
}

template <typename Scalar,int Dim>
void RigidBodyDriver<Scalar, Dim>::advanceFrame()
{
	unsigned int plugin_num = static_cast<unsigned int>((this->plugins_).size());
	RigidDriverPlugin<Scalar, Dim>* plugin;
	for(unsigned int i = 0; i < plugin_num; ++i)
	{
		plugin = dynamic_cast<RigidDriverPlugin<Scalar, Dim>*>((this->plugins_)[i]);
		if(plugin != NULL)
			plugin->onAdvanceFrame();
	}
}

template <typename Scalar,int Dim>
void RigidBodyDriver<Scalar, Dim>::initialize()
{
	unsigned int plugin_num = static_cast<unsigned int>((this->plugins_).size());
	RigidDriverPlugin<Scalar, Dim>* plugin;
	for(unsigned int i = 0; i < plugin_num; ++i)
	{
		plugin = dynamic_cast<RigidDriverPlugin<Scalar, Dim>*>((this->plugins_)[i]);
		if(plugin != NULL)
			plugin->onInitialize();
	}
}

template <typename Scalar,int Dim>
void RigidBodyDriver<Scalar, Dim>::advanceStep(Scalar dt)
{
	collisionDetection();

	unsigned int plugin_num = static_cast<unsigned int>((this->plugins_).size());
	RigidDriverPlugin<Scalar, Dim>* plugin;
	for(unsigned int i = 0; i < plugin_num; ++i)
	{
		plugin = dynamic_cast<RigidDriverPlugin<Scalar, Dim>*>((this->plugins_)[i]);
		if(plugin != NULL)
			plugin->onAdvanceStep(dt);
	}
}

template <typename Scalar,int Dim>
Scalar RigidBodyDriver<Scalar, Dim>::computeTimeStep()
{
	Scalar time_step = static_cast<Scalar>(0);
	return time_step;
}

template <typename Scalar,int Dim>
void RigidBodyDriver<Scalar, Dim>::write(const char *file_name)
{
	unsigned int plugin_num = static_cast<unsigned int>((this->plugins_).size());
	RigidDriverPlugin<Scalar, Dim>* plugin;
	for(unsigned int i = 0; i < plugin_num; ++i)
	{
		plugin = dynamic_cast<RigidDriverPlugin<Scalar, Dim>*>((this->plugins_)[i]);
		if(plugin != NULL)
			plugin->onWrite();
	}
}

template <typename Scalar,int Dim>
void RigidBodyDriver<Scalar, Dim>::read(const char *file_name)
{
	unsigned int plugin_num = static_cast<unsigned int>((this->plugins_).size());
	RigidDriverPlugin<Scalar, Dim>* plugin;
	for(unsigned int i = 0; i < plugin_num; ++i)
	{
		plugin = dynamic_cast<RigidDriverPlugin<Scalar, Dim>*>((this->plugins_)[i]);
		if(plugin != NULL)
			plugin->onRead();
	}
}

template <typename Scalar,int Dim>
void RigidBodyDriver<Scalar, Dim>::addRigidBody(RigidBody<Scalar, Dim>* rigid_body, bool is_rebuild)
{
	if(rigid_body == NULL)
		return;
	RigidBodyArchive<Scalar, Dim>* archive = new RigidBodyArchive<Scalar, Dim>(rigid_body);
	archive->setIndex(numRigidBody());
	scene_bvh_.addObjectBVH(archive->objectBVH(), is_rebuild);
	rigid_body_archives_.push_back(archive);

	unsigned int plugin_num = static_cast<unsigned int>((this->plugins_).size());
	RigidDriverPlugin<Scalar, Dim>* plugin;
	for(unsigned int i = 0; i < plugin_num; ++i)
	{
		plugin = dynamic_cast<RigidDriverPlugin<Scalar, Dim>*>((this->plugins_)[i]);
		if(plugin != NULL)
			plugin->onAddRigidBody(rigid_body);
	}
}

template <typename Scalar,int Dim>
unsigned int RigidBodyDriver<Scalar, Dim>::numRigidBody() const
{
	return static_cast<unsigned int>(rigid_body_archives_.size());
}

template <typename Scalar,int Dim>
RigidBody<Scalar, Dim>* RigidBodyDriver<Scalar, Dim>::rigidBody(unsigned int index)
{
	if(index >= numRigidBody())
	{
		std::cerr<<"Rigid body index out of range!"<<std::endl;
		return NULL;
	}
	return rigid_body_archives_[index]->rigidBody();
}

template <typename Scalar,int Dim>
bool RigidBodyDriver<Scalar, Dim>::collisionDetection()
{
	scene_bvh_.updateSceneBVH();
	return scene_bvh_.selfCollide(collision_result_);
}


//explicit instantiation
template class RigidBodyArchive<float, 3>;
template class RigidBodyArchive<double, 3>;
template class RigidBodyDriver<float, 3>;
template class RigidBodyDriver<double, 3>;


}
