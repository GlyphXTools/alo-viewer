#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include "files.h"
#include "types.h"
#include <string>
#include <vector>

class BoneAnimation;

class Animation
{
	class AnimationImpl;
	
	AnimationImpl* pimpl;

public:
	const BoneAnimation& getBoneAnimation(int index) const;
	unsigned int getNumBoneAnimations() const;
	unsigned int getNumFrames() const;
	float        getFPS() const;

	Animation(File* file, bool infoOnly = false);
	~Animation();
};

class BoneAnimation
{
	friend class Animation::AnimationImpl;

	std::string   name;
	unsigned long boneIndex;
	D3DXVECTOR3   translationOffset;
	std::vector<D3DXQUATERNION> quaternions;
	std::vector<D3DXVECTOR3>    translations;

public:
	const std::string& getName() const { return name; }
	unsigned long getBoneIndex() const { return boneIndex; }
	const D3DXVECTOR3&    getTranslationOffset() const    { return translationOffset;  }
	const D3DXQUATERNION& getQuaternion(int index)  const { return quaternions[index]; }
	const D3DXVECTOR3&    getTranslation(int index) const { return translations[index]; }
	unsigned int getNumQuaternions()  const { return (unsigned int)quaternions.size();  }
	unsigned int getNumTranslations() const { return (unsigned int)translations.size(); }
};

#endif