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

	void readBoneAnimation(File* input, BoneAnimation &animation, bool isFoC, bool infoOnly);
	void readAnimation(File* input, bool infoOnly);

	vector<BoneAnimation> boneAnimations;

public:
	AnimationImpl(File* input, bool infoOnly);
	~AnimationImpl();
};

void Animation::AnimationImpl::readBoneAnimation(File* input, BoneAnimation &animation, bool isFoC, bool infoOnly)
{
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
					unsigned char matsize[9];
					const char*   matdata[9];
					matdata[0] = chunks.getChunk(6,  matsize[0]);
					matdata[1] = chunks.getChunk(7,  matsize[1]);
					matdata[2] = chunks.getChunk(8,  matsize[2]);
					matdata[3] = chunks.getChunk(9,  matsize[3]);
					matdata[4] = chunks.getChunk(14, matsize[4]);
					matdata[5] = chunks.getChunk(15, matsize[5]);
					matdata[6] = chunks.getChunk(16, matsize[6]);
					matdata[7] = chunks.getChunk(17, matsize[7]);

					if (matsize[0] != 12 || matsize[1] != 12 || matsize[2] != 12 || matsize[3] != 12 ||
						(isFoC && (matsize[4] != 2 || matsize[5] != 2 || matsize[6] != 2 || matsize[7] != 8)))
					{
						throw BadFileException(input->getName());
					}
					memcpy(&animation.translationOffset, matdata[0], 3 * sizeof(float));
					memcpy(&animation.translationScale,  matdata[1], 3 * sizeof(float));

					if (isFoC)
					{
						animation.translationBufferIndex = *(int16_t*)matdata[4];
						animation.rotationBufferIndex    = *(int16_t*)matdata[6];

						// Read base quaternion
						animation.quaternions.resize(1);
						float x = (float)*(int16_t*)&matdata[7][0] / INT16_MAX;
						float y = (float)*(int16_t*)&matdata[7][2] / INT16_MAX;
						float z = (float)*(int16_t*)&matdata[7][4] / INT16_MAX;
						float w = (float)*(int16_t*)&matdata[7][6] / INT16_MAX;
						animation.quaternions[0] = D3DXQUATERNION(x,y,z,w);
					}
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
							float x = (float)*(uint16_t*)&data[i*6+0] * animation.translationScale[0];
							float y = (float)*(uint16_t*)&data[i*6+2] * animation.translationScale[1];
							float z = (float)*(uint16_t*)&data[i*6+4] * animation.translationScale[2];
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
				// Visibility track, 1 bit per frame
				break;

			case 0x1008:
				// Unknown track, 1 bit per frame
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
	unsigned long rBlockSize;
	unsigned long tBlockSize;
	bool		  isFoC = false;

	while (!input->eof())
	{
		Chunk chunk(input);
		File* stream = chunk.getStream();

		switch (chunk.getType())
		{
			case 0x1001:
			{
				MiniChunks chunks(chunk);
				unsigned char size[6];
				const char* data[6];
				data[0] = chunks.getChunk(1, size[0]);
				data[1] = chunks.getChunk(2, size[1]);
				data[2] = chunks.getChunk(3, size[2]);
				data[3] = chunks.getChunk(11, size[3]);
				data[4] = chunks.getChunk(12, size[4]);
				data[5] = chunks.getChunk(13, size[5]);
				isFoC = (data[3] != NULL || data[4] != NULL || data[5] != NULL);

				if (size[0] != 4 || size[1] != 4 || size[2] != 4 || (isFoC && (size[3] != 4 || size[4] != 4 || size[5] != 4)))
				{
					throw BadFileException(input->getName());
				}
				this->nFrames = *(unsigned long*)data[0];
				this->fps     = *(float*)data[1];
				this->nBones  = *(unsigned long*)data[2];
				boneAnimations.resize(this->nBones);
				if (isFoC)
				{
					rBlockSize = *(unsigned long*)data[3];
					tBlockSize = *(unsigned long*)data[4];
				}
				break;
			}

			case 0x1002:
				readBoneAnimation(stream, boneAnimations[nBones++], isFoC, infoOnly);
				break;

			case 0x1009:
				if (!isFoC || nFrames * rBlockSize * sizeof(int16_t) != chunk.getSize())
				{
					throw BadFileException(input->getName());
				}

				if (!infoOnly)
				{
					int16_t* data = (int16_t*)chunk.getData();
					for (vector<BoneAnimation>::iterator animation = boneAnimations.begin(); animation != boneAnimations.end(); animation++)
					{
						if (animation->rotationBufferIndex != -1)
						{
							animation->quaternions.resize(nFrames);
							for (unsigned long i = 0; i < nFrames; i++)
							{
								int index = i * rBlockSize + animation->rotationBufferIndex;
								float x = (float)data[index+0] / INT16_MAX;
								float y = (float)data[index+1] / INT16_MAX;
								float z = (float)data[index+2] / INT16_MAX;
								float w = (float)data[index+3] / INT16_MAX;
								animation->quaternions[i] = D3DXQUATERNION(x,y,z,w);
							}
						}
					}
					delete[] data;
				}
				break;

			case 0x100A:
				if (!isFoC || nFrames * tBlockSize * sizeof(uint16_t) != chunk.getSize())
				{
					throw BadFileException(input->getName());
				}

				if (!infoOnly)
				{
					uint16_t* data = (uint16_t*)chunk.getData();
					for (vector<BoneAnimation>::iterator animation = boneAnimations.begin(); animation != boneAnimations.end(); animation++)
					{
						if (animation->translationBufferIndex != -1)
						{
							animation->translations.resize(nFrames);
							for (unsigned long i = 0; i < nFrames; i++)
							{
								int index = i * tBlockSize + animation->translationBufferIndex;
								float x = (float)data[index+0] * animation->translationScale[0];
								float y = (float)data[index+1] * animation->translationScale[1];
								float z = (float)data[index+2] * animation->translationScale[2];
								animation->translations[i] = D3DXVECTOR3(x,y,z);
							}
						}
					}
					delete[] data;
				}
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