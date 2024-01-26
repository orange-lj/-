#include"api.h"
#include <bcrypt.h>

#define KPH_SIGN_ALGORITHM BCRYPT_ECDSA_P256_ALGORITHM
#define KPH_SIGN_ALGORITHM_BITS 256
#define KPH_HASH_ALGORITHM BCRYPT_SHA256_ALGORITHM
#define KPH_BLOB_PUBLIC BCRYPT_ECCPUBLIC_BLOB

typedef struct {
    BCRYPT_ALG_HANDLE algHandle;
    BCRYPT_HASH_HANDLE handle;
    PVOID object;
} MY_HASH_OBJ;

NTSTATUS MyInitHash(MY_HASH_OBJ* pHashObj) 
{
    NTSTATUS status;
    ULONG hashObjectSize;
    ULONG querySize;
    memset(pHashObj, 0, sizeof(MY_HASH_OBJ));
    if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider(&pHashObj->algHandle, KPH_HASH_ALGORITHM, NULL, 0)))
        goto CleanupExit;

    if (!NT_SUCCESS(status = BCryptGetProperty(pHashObj->algHandle, BCRYPT_OBJECT_LENGTH, (PUCHAR)&hashObjectSize, sizeof(ULONG), &querySize, 0)))
        goto CleanupExit;

    pHashObj->object = ExAllocatePoolWithTag(PagedPool, hashObjectSize, 'vhpK');
    if (!pHashObj->object) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = BCryptCreateHash(pHashObj->algHandle, &pHashObj->handle, (PUCHAR)pHashObj->object, hashObjectSize, NULL, 0, 0)))
        goto CleanupExit;

CleanupExit:
    //失败时调用者必须调用MyFreeHash

    return status;
}

NTSTATUS MyHashData(MY_HASH_OBJ* pHashObj, PVOID Data, ULONG DataSize)
{
    return BCryptHashData(pHashObj->handle, (PUCHAR)Data, DataSize, 0);
}

NTSTATUS MyFinishHash(MY_HASH_OBJ* pHashObj, PVOID* Hash, PULONG HashSize)
{
    NTSTATUS status;
    ULONG querySize;

    if (!NT_SUCCESS(status = BCryptGetProperty(pHashObj->algHandle, BCRYPT_HASH_LENGTH, (PUCHAR)HashSize, sizeof(ULONG), &querySize, 0)))
        goto CleanupExit;

    *Hash = ExAllocatePoolWithTag(PagedPool, *HashSize, 'vhpK');
    if (!*Hash) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = BCryptFinishHash(pHashObj->handle, (PUCHAR)*Hash, *HashSize, 0)))
        goto CleanupExit;

    return STATUS_SUCCESS;

CleanupExit:
    if (*Hash) {
        ExFreePoolWithTag(*Hash, 'vhpK');
        *Hash = NULL;
    }

    return status;
}

//NTSTATUS KphVerifySignature(
//    _In_ PVOID Hash,
//    _In_ ULONG HashSize,
//    _In_ PUCHAR Signature,
//    _In_ ULONG SignatureSize
//)
//{
//    NTSTATUS status;
//    BCRYPT_ALG_HANDLE signAlgHandle = NULL;
//    BCRYPT_KEY_HANDLE keyHandle = NULL;
//    PVOID hash = NULL;
//    ULONG hashSize;
//
//    // Import the trusted public key.
//
//    if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider(&signAlgHandle, KPH_SIGN_ALGORITHM, NULL, 0)))
//        goto CleanupExit;
//    if (!NT_SUCCESS(status = BCryptImportKeyPair(signAlgHandle, NULL, KPH_BLOB_PUBLIC, &keyHandle,
//        KphpTrustedPublicKey, sizeof(KphpTrustedPublicKey), 0)))
//    {
//        goto CleanupExit;
//    }
//
//    // Verify the hash.
//
//    if (!NT_SUCCESS(status = BCryptVerifySignature(keyHandle, NULL, Hash, HashSize, Signature,
//        SignatureSize, 0)))
//    {
//        goto CleanupExit;
//    }
//
//CleanupExit:
//    if (keyHandle)
//        BCryptDestroyKey(keyHandle);
//    if (signAlgHandle)
//        BCryptCloseAlgorithmProvider(signAlgHandle, 0);
//
//    return status;
//}

NTSTATUS KphVerifyBuffer(PUCHAR Buffer, ULONG BufferSize, PUCHAR Signature, ULONG SignatureSize) 
{
    NTSTATUS status;
    MY_HASH_OBJ hashObj;
    PVOID hash = NULL;
    ULONG hashSize;
    //对缓冲区进行哈希运算。
    if (!NT_SUCCESS(status = MyInitHash(&hashObj)))
        goto CleanupExit;

    MyHashData(&hashObj, Buffer, BufferSize);

    if (!NT_SUCCESS(status = MyFinishHash(&hashObj, &hash, &hashSize)))
        goto CleanupExit;

    //验证哈希值。

    /*if (!NT_SUCCESS(status = KphVerifySignature(hash, hashSize, Signature, SignatureSize)))
    {
        goto CleanupExit;
    }*/

CleanupExit:
    if (hash)
        ExFreePoolWithTag(hash, 'vhpK');

    return status;

}
