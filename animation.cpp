#include "animation.h"
#include "exceptions.h"
#include "chunks.h"

using namespace std;

class Animation::AnimationImpl
{
	friend class Animation;

	unsigned long nFrames;
	float         fps;
	unsigned long nBones;

	void readBoneAnimation(File* input, BoneAnimation &animation, bool infoOnly);
	void readAnimation(File* input, bool infoOnly);

	vector<BoneAnimation> boneAnimations;

public:
	AnimationImpl(File* input, bool infoOnly);
	~AnimationImpl();
};

void Animation::AnimationImpl::readBoneAnimation(File* input, BoneAnimation &animation, bool infoOnly)
{
	float translationScale[3];

	bool readData = false;

	while (!input->eof())
	{
		Chunk chunk(input);
		File* stream = chunk.getStream();

		switch (chunk.getType())
		{
			case 0x1003:
			{
				MiniChunks chunks(chunk);
				unsigned char size;
				const char*   data;

				// Read name
				data = chunks.getChunk(4, size);
				if (data == NULL)
				{
					throw BadFileException(input->getName());
				}
				animation.name = data;

				// Read bone index
				data = chunks.getChunk(5, size);
				if (data == NULL || size != 4)
				{
					throw BadFileException(input->getName());
				}
				animation.boneIndex = *(unsigned long*)data;

				if (!infoOnly)
				{
					// Read base matrix
					unsigned char matsize[4];
					const char*   matdata[4];
					matdata[0] = chunks.getChunk(6, matsize[0]);
					matdata[1] = chunks.getChunk(7, matsize[1]);
					matdata[2] = chunks.getChunk(8, matsize[2]);
					matdata[3] = chunks.getChunk(9, matsize[3]);
					if (matsize[0] != 12 || matsize[1] != 12 || matsize[2] != 12 || matsize[3] != 12)
					{
						throw BadFileException(input->getName());
					}
					memcpy(&animation.translationOffset, matdata[0], 3 * sizeof(float));
					memcpy(&translationScale, matdata[1], 3 * sizeof(float));
				}
				readData = true;
				break;
			}

			case 0x1004:
			{
				if (!infoOnly)
				{
					unsigned long size  = chunk.getSize();
					unsigned long count = size / 6;
					if (size > 0)
					{
						if (size != 6 * nFrames)
						{
							throw BadFileException(input->getName());
						}
						const char* data = chunk.getData();
						animation.translations.resize(nFrames);
						for (unsigned long i = 0; i < nFrames; i++)
						{
							float x = (float)*(uint16_t*)&data[i*6+0] * translationScale[0];
							float y = (float)*(uint16_t*)&data[i*6+2] * translationScale[1];
							float z = (float)*(uint16_t*)&data[i*6+4] * translationScale[2];
							animation.translations[i] = D3DXVECTOR3(x,y,z);
						}
						delete[] data;
					}
				}
				break;
			}

			case 0x1006:
			{
				if (!infoOnly)
				{
					unsigned long size  = chunk.getSize();
					unsigned long count = size / 8;
					if (size != 8 && size != 8 * nFrames)
					{
						throw BadFileException(input->getName());
					}
					const char* data = chunk.getData();
					animation.quaternions.resize(count);
					for (unsigned long i = 0; i < count; i++)
					{
						float x = (float)*(int16_t*)&data[i*8+0] / INT16_MAX;
						float y = (float)*(int16_t*)&data[i*8+2] / INT16_MAX;
						float z = (float)*(int16_t*)&data[i*8+4] / INT16_MAX;
						float w = (float)*(int16_t*)&data[i*8+6] / INT16_MAX;
						animation.quaternions[i] = D3DXQUATERNION(x,y,z,w);
					}
					delete[] data;
				}
				break;
			}

			case 0x1007:
				break;

			case 0x1008:
				break;

			default:
				throw BadFileException(input->getName());
				break;
		}
		input->seek(chunk.getStart() + chunk.getSize());
	}

	if (!readData)
	{
		throw BadFileException(input->getName());
	}
}

void Animation::AnimationImpl::readAnimation(File* input, bool infoOnly)
{
	unsigned long nBones = 0;

	while (!input->eof())
	{
		Chunk chunk(input);
		File* stream = chunk.getStream();

		switch (chunk.getType())
		{
			case 0x1001:
			{
				MiniChunks chunks(chunk);
				unsigned char size[3];
				const char* data[3];
				data[0] = chunks.getChunk(1, size[0]);
				data[1] = chunks.getChunk(2, size[1]);
				data[2] = chunks.getChunk(3, size[2]);
				if (size[0] != 4 || size[1] != 4 || size[2] != 4)
				{
					throw BadFileException(input->getName());
				}
				this->nFrames = *(unsigned long*)data[0];
				this->fps     = *(float*)data[1];
				this->nBones  = *(unsigned long*)data[2];
				boneAnimations.resize(this->nBones);
				break;
			}

			case 0x1002:
				readBoneAnimation(stream, boneAnimations[nBones++], infoOnly);
				break;

			default:
				throw BadFileException(input->getName());
				break;
		}
		input->seek(chunk.getStart() + chunk.getSize());
	}

	if (this->nBones != nBones)
	{
		throw BadFileException(input->getName());
	}
}

Animation::AnimationImpl::AnimationImpl(File* input, bool infoOnly)
{
	while (!input->eof())
	{
		Chunk chunk(input);
		File* stream = chunk.getStream();

		switch (chunk.getType())
		{
			case 0x1000:
				readAnimation(stream, infoOnly);
				break;

			default:
				throw BadFileException(input->getName());
				break;
		}
		input->seek(chunk.getStart() + chunk.getSize());
	}
}

Animation::AnimationImpl::~AnimationImpl()
{
}

const BoneAnimation& Animation::getBoneAnimation(int index) const
{
	return pimpl->boneAnimations[index];
}

unsigned int Animation::getNumBoneAnimations() const
{
	return (unsigned int)pimpl->boneAnimations.size();
}

unsigned int Animation::getNumFrames() const
{
	return pimpl->nFrames;
}

float Animation::getFPS() const
{
	return pimpl->fps;
}

Animation::Animation(File* file, bool infoOnly) : pimpl(new AnimationImpl(file, infoOnly))
{
}

Animation::~Animation()
{
	delete pimpl;
}